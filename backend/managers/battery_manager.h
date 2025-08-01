#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QSettings>
#include <memory>

namespace FRCDriverStation {

class Logger;

/**
 * @brief Manages battery voltage monitoring and alerts
 * 
 * This manager:
 * - Monitors robot battery voltage levels
 * - Provides configurable voltage thresholds
 * - Emits alerts for critical battery levels
 * - Maintains voltage history for analysis
 * - Can automatically disable robot on critical voltage
 * 
 * Design principles:
 * - Safety first: Always err on the side of caution
 * - Configurable: Allow teams to set their own thresholds
 * - Observable: Clear alerts and status updates
 * - Historical: Maintain data for analysis
 */
class BatteryManager : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(double currentVoltage READ currentVoltage NOTIFY currentVoltageChanged)
    Q_PROPERTY(double criticalThreshold READ criticalThreshold WRITE setCriticalThreshold NOTIFY criticalThresholdChanged)
    Q_PROPERTY(double warningThreshold READ warningThreshold WRITE setWarningThreshold NOTIFY warningThresholdChanged)
    Q_PROPERTY(bool autoDisableEnabled READ autoDisableEnabled WRITE setAutoDisableEnabled NOTIFY autoDisableEnabledChanged)
    Q_PROPERTY(QString batteryStatus READ batteryStatus NOTIFY batteryStatusChanged)

public:
    enum BatteryLevel {
        Critical = 0,
        Warning = 1,
        Normal = 2,
        Unknown = 3
    };
    Q_ENUM(BatteryLevel)

    explicit BatteryManager(std::shared_ptr<Logger> logger, QObject *parent = nullptr);
    ~BatteryManager();

    // Property getters
    double currentVoltage() const { return m_currentVoltage; }
    double criticalThreshold() const { return m_criticalThreshold; }
    double warningThreshold() const { return m_warningThreshold; }
    bool autoDisableEnabled() const { return m_autoDisableEnabled; }
    QString batteryStatus() const { return m_batteryStatus; }
    BatteryLevel batteryLevel() const { return m_batteryLevel; }

    // Configuration
    void setCriticalThreshold(double threshold);
    void setWarningThreshold(double threshold);
    void setAutoDisableEnabled(bool enabled);

    // Data access
    Q_INVOKABLE QList<double> getVoltageHistory(int seconds = 60) const;
    Q_INVOKABLE double getAverageVoltage(int seconds = 10) const;
    Q_INVOKABLE double getMinimumVoltage(int seconds = 60) const;

    // Settings management
    void loadSettings(QSettings *settings);
    void saveSettings(QSettings *settings);

    // Data update (called by RobotState)
    void updateVoltage(double voltage);

signals:
    void currentVoltageChanged(double voltage);
    void criticalThresholdChanged(double threshold);
    void warningThresholdChanged(double threshold);
    void autoDisableEnabledChanged(bool enabled);
    void batteryStatusChanged(const QString &status);
    void batteryLevelChanged(BatteryLevel level);
    void voltageAlert(BatteryLevel level, double voltage);
    void robotShouldDisable();

private slots:
    void checkBatteryLevel();

private:
    void updateBatteryLevel();
    void updateBatteryStatus();

    std::shared_ptr<Logger> m_logger;
    std::unique_ptr<QTimer> m_checkTimer;

    // Current state
    double m_currentVoltage;
    BatteryLevel m_batteryLevel;
    QString m_batteryStatus;

    // Configuration
    double m_criticalThreshold;
    double m_warningThreshold;
    bool m_autoDisableEnabled;

    // History tracking
    struct VoltageReading {
        qint64 timestamp;
        double voltage;
    };
    QList<VoltageReading> m_voltageHistory;
    static constexpr int MAX_HISTORY_SIZE = 3600; // 1 hour at 1Hz
};

} // namespace FRCDriverStation

#endif // BATTERY_MANAGER_H
