#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHostAddress>
#include <memory>

namespace FRCDriverStation {

class Logger;

/**
 * @brief Manages network-related functionality and diagnostics
 * 
 * This manager provides:
 * - Network interface monitoring
 * - Internet connectivity checking
 * - Network performance metrics
 * - Router/gateway detection
 * - Network troubleshooting tools
 * 
 * Design principles:
 * - Non-intrusive: Don't interfere with robot communication
 * - Informative: Provide useful diagnostic information
 * - Efficient: Minimize network overhead
 * - Reliable: Handle network failures gracefully
 */
class NetworkManager : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(bool internetConnected READ internetConnected NOTIFY internetConnectedChanged)
    Q_PROPERTY(QString primaryInterface READ primaryInterface NOTIFY primaryInterfaceChanged)
    Q_PROPERTY(QStringList availableInterfaces READ availableInterfaces NOTIFY availableInterfacesChanged)
    Q_PROPERTY(QString gatewayAddress READ gatewayAddress NOTIFY gatewayAddressChanged)

public:
    explicit NetworkManager(std::shared_ptr<Logger> logger, QObject *parent = nullptr);
    ~NetworkManager();

    // Property getters
    bool internetConnected() const { return m_internetConnected; }
    QString primaryInterface() const { return m_primaryInterface; }
    QStringList availableInterfaces() const { return m_availableInterfaces; }
    QString gatewayAddress() const { return m_gatewayAddress; }

    // Network diagnostics
    Q_INVOKABLE void refreshNetworkInfo();
    Q_INVOKABLE bool pingHost(const QString &host, int timeout = 1000);
    Q_INVOKABLE QStringList getInterfaceAddresses(const QString &interfaceName);
    Q_INVOKABLE QString getInterfaceStatus(const QString &interfaceName);

signals:
    void internetConnectedChanged(bool connected);
    void primaryInterfaceChanged(const QString &interface);
    void availableInterfacesChanged(const QStringList &interfaces);
    void gatewayAddressChanged(const QString &address);
    void networkStatusChanged(const QString &status);

private slots:
    void checkInternetConnectivity();
    void onConnectivityCheckFinished();
    void updateNetworkInterfaces();

private:
    void detectGateway();
    void updatePrimaryInterface();

    std::shared_ptr<Logger> m_logger;
    std::unique_ptr<QTimer> m_checkTimer;
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    QNetworkReply *m_connectivityReply;

    // Network state
    bool m_internetConnected;
    QString m_primaryInterface;
    QStringList m_availableInterfaces;
    QString m_gatewayAddress;
};

} // namespace FRCDriverStation

#endif // NETWORK_MANAGER_H
