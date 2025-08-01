#include "battery_manager.h"
#include "../core/logger.h"
#include "../core/constants.h"
#include <QDateTime>
#include <algorithm>

using namespace FRCDriverStation;
using namespace FRCDriverStation::Constants;

BatteryManager::BatteryManager(std::shared_ptr<Logger> logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_checkTimer(std::make_unique<QTimer>(this))
    , m_currentVoltage(0.0)
    , m_batteryLevel(Unknown)
    , m_batteryStatus("Unknown")
    , m_criticalThreshold(Battery::CRITICAL_VOLTAGE)
    , m_warningThreshold(Battery::WARNING_VOLTAGE)
    , m_autoDisableEnabled(true)
{
    // Setup check timer (1Hz)
    m_checkTimer->setInterval(1000);
    connect(m_checkTimer.get(), &QTimer::timeout, this, &BatteryManager::checkBatteryLevel);
    m_checkTimer->start();

    m_logger->info("Battery Manager", "Battery manager initialized");
}

BatteryManager::~BatteryManager()
{
    m_logger->info("Battery Manager", "Battery manager destroyed");
}

void BatteryManager::setCriticalThreshold(double threshold)
{
    if (qAbs(m_criticalThreshold - threshold) > 0.01) {
        m_criticalThreshold = threshold;
        emit criticalThresholdChanged(threshold);
        updateBatteryLevel();
        
        m_logger->info("Battery Manager", "Critical threshold changed", 
                      QString("%1V").arg(threshold, 0, 'f', 2));
    }
}

void BatteryManager::setWarningThreshold(double threshold)
{
    if (qAbs(m_warningThreshold - threshold) > 0.01) {
        m_warningThreshold = threshold;
        emit warningThresholdChanged(threshold);
        updateBatteryLevel();
        
        m_logger->info("Battery Manager", "Warning threshold changed", 
                      QString("%1V").arg(threshold, 0, 'f', 2));
    }
}

void BatteryManager::setAutoDisableEnabled(bool enabled)
{
    if (m_autoDisableEnabled != enabled) {
        m_autoDisableEnabled = enabled;
        emit autoDisableEnabledChanged(enabled);
        
        m_logger->info("Battery Manager", "Auto-disable changed", 
                      enabled ? "Enabled" : "Disabled");
    }
}

QList<double> BatteryManager::getVoltageHistory(int seconds) const
{
    QList<double> history;
    qint64 cutoffTime = QDateTime::currentMSecsSinceEpoch() - (seconds * 1000);
    
    for (const VoltageReading &reading : m_voltageHistory) {
        if (reading.timestamp >= cutoffTime) {
            history.append(reading.voltage);
        }
    }
    
    return history;
}

double BatteryManager::getAverageVoltage(int seconds) const
{
    QList<double> history = getVoltageHistory(seconds);
    
    if (history.isEmpty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double voltage : history) {
        sum += voltage;
    }
    
    return sum / history.size();
}

double BatteryManager::getMinimumVoltage(int seconds) const
{
    QList<double> history = getVoltageHistory(seconds);
    
    if (history.isEmpty()) {
        return 0.0;
    }
    
    return *std::min_element(history.begin(), history.end());
}

void BatteryManager::loadSettings(QSettings *settings)
{
    settings->beginGroup("BatteryManager");
    
    m_criticalThreshold = settings->value("criticalThreshold", Battery::CRITICAL_VOLTAGE).toDouble();
    m_warningThreshold = settings->value("warningThreshold", Battery::WARNING_VOLTAGE).toDouble();
    m_autoDisableEnabled = settings->value("autoDisableEnabled", true).toBool();
    
    settings->endGroup();
    
    // Emit signals to update UI
    emit criticalThresholdChanged(m_criticalThreshold);
    emit warningThresholdChanged(m_warningThreshold);
    emit autoDisableEnabledChanged(m_autoDisableEnabled);
    
    m_logger->debug("Battery Manager", "Settings loaded");
}

void BatteryManager::saveSettings(QSettings *settings)
{
    settings->beginGroup("BatteryManager");
    
    settings->setValue("criticalThreshold", m_criticalThreshold);
    settings->setValue("warningThreshold", m_warningThreshold);
    settings->setValue("autoDisableEnabled", m_autoDisableEnabled);
    
    settings->endGroup();
    
    m_logger->debug("Battery Manager", "Settings saved");
}

void BatteryManager::updateVoltage(double voltage)
{
    if (qAbs(m_currentVoltage - voltage) > Battery::VOLTAGE_CHANGE_THRESHOLD) {
        m_currentVoltage = voltage;
        emit currentVoltageChanged(voltage);
        
        // Add to history
        VoltageReading reading;
        reading.timestamp = QDateTime::currentMSecsSinceEpoch();
        reading.voltage = voltage;
        m_voltageHistory.append(reading);
        
        // Trim history to maximum size
        while (m_voltageHistory.size() > MAX_HISTORY_SIZE) {
            m_voltageHistory.removeFirst();
        }
        
        updateBatteryLevel();
    }
}

void BatteryManager::checkBatteryLevel()
{
    // This is called periodically to check for sustained low voltage
    if (m_batteryLevel == Critical && m_autoDisableEnabled) {
        // Check if voltage has been critical for more than 2 seconds
        double avgVoltage = getAverageVoltage(2);
        if (avgVoltage > 0.0 && avgVoltage <= m_criticalThreshold) {
            m_logger->critical("Battery Manager", "Sustained critical voltage detected", 
                             QString("Average: %1V over 2 seconds").arg(avgVoltage, 0, 'f', 2));
            emit robotShouldDisable();
        }
    }
}

void BatteryManager::updateBatteryLevel()
{
    BatteryLevel newLevel = Unknown;
    
    if (m_currentVoltage <= 0.0) {
        newLevel = Unknown;
    } else if (m_currentVoltage <= m_criticalThreshold) {
        newLevel = Critical;
    } else if (m_currentVoltage <= m_warningThreshold) {
        newLevel = Warning;
    } else {
        newLevel = Normal;
    }
    
    if (m_batteryLevel != newLevel) {
        BatteryLevel oldLevel = m_batteryLevel;
        m_batteryLevel = newLevel;
        emit batteryLevelChanged(newLevel);
        
        // Log level changes
        QStringList levelNames = {"Critical", "Warning", "Normal", "Unknown"};
        m_logger->info("Battery Level", "Battery level changed", 
                      QString("From %1 to %2 (%3V)")
                      .arg(levelNames[oldLevel])
                      .arg(levelNames[newLevel])
                      .arg(m_currentVoltage, 0, 'f', 2));
        
        // Emit alerts for critical and warning levels
        if (newLevel == Critical || newLevel == Warning) {
            emit voltageAlert(newLevel, m_currentVoltage);
        }
        
        updateBatteryStatus();
    }
}

void BatteryManager::updateBatteryStatus()
{
    QString newStatus;
    
    switch (m_batteryLevel) {
        case Critical:
            newStatus = QString("CRITICAL (%1V)").arg(m_currentVoltage, 0, 'f', 2);
            break;
        case Warning:
            newStatus = QString("Warning (%1V)").arg(m_currentVoltage, 0, 'f', 2);
            break;
        case Normal:
            newStatus = QString("Normal (%1V)").arg(m_currentVoltage, 0, 'f', 2);
            break;
        case Unknown:
        default:
            newStatus = "Unknown";
            break;
    }
    
    if (m_batteryStatus != newStatus) {
        m_batteryStatus = newStatus;
        emit batteryStatusChanged(newStatus);
    }
}
