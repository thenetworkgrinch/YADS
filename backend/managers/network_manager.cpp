#include "network_manager.h"
#include "../core/logger.h"
#include <QNetworkInterface>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

using namespace FRCDriverStation;

NetworkManager::NetworkManager(std::shared_ptr<Logger> logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_checkTimer(std::make_unique<QTimer>(this))
    , m_networkManager(std::make_unique<QNetworkAccessManager>(this))
    , m_connectivityReply(nullptr)
    , m_internetConnected(false)
{
    // Setup periodic network checks (every 30 seconds)
    m_checkTimer->setInterval(30000);
    connect(m_checkTimer.get(), &QTimer::timeout, this, &NetworkManager::checkInternetConnectivity);
    connect(m_checkTimer.get(), &QTimer::timeout, this, &NetworkManager::updateNetworkInterfaces);
    m_checkTimer->start();

    // Initial network info gathering
    refreshNetworkInfo();

    m_logger->info("Network Manager", "Network manager initialized");
}

NetworkManager::~NetworkManager()
{
    if (m_connectivityReply) {
        m_connectivityReply->abort();
    }
    
    m_logger->info("Network Manager", "Network manager destroyed");
}

void NetworkManager::refreshNetworkInfo()
{
    updateNetworkInterfaces();
    detectGateway();
    checkInternetConnectivity();
}

bool NetworkManager::pingHost(const QString &host, int timeout)
{
    QProcess pingProcess;
    QStringList arguments;
    
#ifdef Q_OS_WIN
    arguments << "-n" << "1" << "-w" << QString::number(timeout) << host;
    pingProcess.start("ping", arguments);
#else
    arguments << "-c" << "1" << "-W" << QString::number(timeout / 1000) << host;
    pingProcess.start("ping", arguments);
#endif
    
    if (!pingProcess.waitForFinished(timeout + 1000)) {
        return false;
    }
    
    return pingProcess.exitCode() == 0;
}

QStringList NetworkManager::getInterfaceAddresses(const QString &interfaceName)
{
    QStringList addresses;
    
    QNetworkInterface interface = QNetworkInterface::interfaceFromName(interfaceName);
    if (interface.isValid()) {
        QList<QNetworkAddressEntry> entries = interface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                addresses.append(entry.ip().toString());
            }
        }
    }
    
    return addresses;
}

QString NetworkManager::getInterfaceStatus(const QString &interfaceName)
{
    QNetworkInterface interface = QNetworkInterface::interfaceFromName(interfaceName);
    if (!interface.isValid()) {
        return "Invalid";
    }
    
    if (!(interface.flags() & QNetworkInterface::IsUp)) {
        return "Down";
    }
    
    if (!(interface.flags() & QNetworkInterface::IsRunning)) {
        return "Not Running";
    }
    
    QList<QNetworkAddressEntry> entries = interface.addressEntries();
    bool hasIPv4 = false;
    for (const QNetworkAddressEntry &entry : entries) {
        if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol &&
            !entry.ip().isLoopback()) {
            hasIPv4 = true;
            break;
        }
    }
    
    if (!hasIPv4) {
        return "No IP Address";
    }
    
    return "Connected";
}

void NetworkManager::checkInternetConnectivity()
{
    if (m_connectivityReply) {
        return; // Check already in progress
    }
    
    // Use a lightweight connectivity check
    QNetworkRequest request(QUrl("http://www.msftconnecttest.com/connecttest.txt"));
    request.setRawHeader("User-Agent", "FRC-DriverStation");
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    
    m_connectivityReply = m_networkManager->get(request);
    connect(m_connectivityReply, &QNetworkReply::finished, this, &NetworkManager::onConnectivityCheckFinished);
    
    // Set timeout
    QTimer::singleShot(5000, this, [this]() {
        if (m_connectivityReply) {
            m_connectivityReply->abort();
        }
    });
}

void NetworkManager::onConnectivityCheckFinished()
{
    bool connected = false;
    
    if (m_connectivityReply->error() == QNetworkReply::NoError) {
        QByteArray data = m_connectivityReply->readAll();
        connected = data.contains("Microsoft Connect Test");
    }
    
    if (m_internetConnected != connected) {
        m_internetConnected = connected;
        emit internetConnectedChanged(connected);
        
        m_logger->info("Network Manager", "Internet connectivity changed", 
                      connected ? "Connected" : "Disconnected");
    }
    
    m_connectivityReply->deleteLater();
    m_connectivityReply = nullptr;
}

void NetworkManager::updateNetworkInterfaces()
{
    QStringList newInterfaces;
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    
    for (const QNetworkInterface &interface : interfaces) {
        // Skip loopback and non-active interfaces
        if (interface.flags() & QNetworkInterface::IsLoopBack) {
            continue;
        }
        
        if (!(interface.flags() & QNetworkInterface::IsUp)) {
            continue;
        }
        
        // Check if interface has IPv4 address
        bool hasIPv4 = false;
        QList<QNetworkAddressEntry> entries = interface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol &&
                !entry.ip().isLoopback()) {
                hasIPv4 = true;
                break;
            }
        }
        
        if (hasIPv4) {
            newInterfaces.append(interface.name());
        }
    }
    
    if (m_availableInterfaces != newInterfaces) {
        m_availableInterfaces = newInterfaces;
        emit availableInterfacesChanged(newInterfaces);
        
        updatePrimaryInterface();
        
        m_logger->debug("Network Manager", "Available interfaces updated", 
                       newInterfaces.join(", "));
    }
}

void NetworkManager::detectGateway()
{
    QString newGateway;
    
#ifdef Q_OS_WIN
    QProcess process;
    process.start("route", QStringList() << "print" << "0.0.0.0");
    if (process.waitForFinished(3000)) {
        QString output = process.readAllStandardOutput();
        QRegularExpression regex(R"(0\.0\.0\.0\s+0\.0\.0\.0\s+(\d+\.\d+\.\d+\.\d+))");
        QRegularExpressionMatch match = regex.match(output);
        if (match.hasMatch()) {
            newGateway = match.captured(1);
        }
    }
#else
    QProcess process;
    process.start("ip", QStringList() << "route" << "show" << "default");
    if (process.waitForFinished(3000)) {
        QString output = process.readAllStandardOutput();
        QRegularExpression regex(R"(default via (\d+\.\d+\.\d+\.\d+))");
        QRegularExpressionMatch match = regex.match(output);
        if (match.hasMatch()) {
            newGateway = match.captured(1);
        }
    }
#endif
    
    if (m_gatewayAddress != newGateway) {
        m_gatewayAddress = newGateway;
        emit gatewayAddressChanged(newGateway);
        
        m_logger->debug("Network Manager", "Gateway address updated", newGateway);
    }
}

void NetworkManager::updatePrimaryInterface()
{
    QString newPrimary;
    
    // Find the interface with the default route
    if (!m_availableInterfaces.isEmpty()) {
        // For now, just use the first available interface
        // In a more sophisticated implementation, we would check routing tables
        newPrimary = m_availableInterfaces.first();
        
        // Prefer Ethernet over WiFi if both are available
        for (const QString &interface : m_availableInterfaces) {
            if (interface.contains("eth", Qt::CaseInsensitive) ||
                interface.contains("en", Qt::CaseInsensitive)) {
                newPrimary = interface;
                break;
            }
        }
    }
    
    if (m_primaryInterface != newPrimary) {
        m_primaryInterface = newPrimary;
        emit primaryInterfaceChanged(newPrimary);
        
        m_logger->info("Network Manager", "Primary interface changed", newPrimary);
    }
}
