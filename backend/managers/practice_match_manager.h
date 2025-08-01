#ifndef PRACTICE_MATCH_MANAGER_H
#define PRACTICE_MATCH_MANAGER_H

#include "../core/constants.h"

#ifdef ENABLE_PRACTICE_MATCH

#include <QObject>
#include <QTimer>
#include <QSettings>
#include <memory>

namespace FRCDriverStation {

class Logger;

/**
 * @brief Manages practice match timing and control
 * 
 * This manager provides:
 * - Configurable match timing (autonomous, teleop, endgame)
 * - Automatic mode transitions during practice matches
 * - Match state management and control
 * - Integration with robot control system
 * 
 * Design principles:
 * - Predictable: Match timing should be consistent and reliable
 * - Configurable: Teams should be able to adjust timing
 * - Observable: Clear state transitions and notifications
 * - Safe: Never interfere with real FMS control
 */
class PracticeMatchManager : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(MatchPhase currentPhase READ currentPhase NOTIFY currentPhaseChanged)
    Q_PROPERTY(int timeRemaining READ timeRemaining NOTIFY timeRemainingChanged)
    Q_PROPERTY(int autonomousTime READ autonomousTime WRITE setAutonomousTime NOTIFY autonomousTimeChanged)
    Q_PROPERTY(int teleopTime READ teleopTime WRITE setTeleopTime NOTIFY teleopTimeChanged)
    Q_PROPERTY(int endgameTime READ endgameTime WRITE setEndgameTime NOTIFY endgameTimeChanged)
    Q_PROPERTY(bool autoStartEnabled READ autoStartEnabled WRITE setAutoStartEnabled NOTIFY autoStartEnabledChanged)

public:
    enum MatchPhase {
        PreMatch = 0,
        Autonomous = 1,
        Teleop = 2,
        Endgame = 3,
        PostMatch = 4
    };
    Q_ENUM(MatchPhase)

    explicit PracticeMatchManager(std::shared_ptr<Logger> logger, QObject *parent = nullptr);
    ~PracticeMatchManager();

    // Property getters
    bool running() const { return m_running; }
    MatchPhase currentPhase() const { return m_currentPhase; }
    int timeRemaining() const { return m_timeRemaining; }
    int autonomousTime() const { return m_autonomousTime; }
    int teleopTime() const { return m_teleopTime; }
    int endgameTime() const { return m_endgameTime; }
    bool autoStartEnabled() const { return m_autoStartEnabled; }

    // Configuration
    void setAutonomousTime(int seconds);
    void setTeleopTime(int seconds);
    void setEndgameTime(int seconds);
    void setAutoStartEnabled(bool enabled);

    // Match control
    Q_INVOKABLE void startMatch();
    Q_INVOKABLE void stopMatch();
    Q_INVOKABLE void pauseMatch();
    Q_INVOKABLE void resumeMatch();
    Q_INVOKABLE void resetMatch();

    // Settings management
    void loadSettings(QSettings *settings);
    void saveSettings(QSettings *settings);

    // External control (called by RobotState)
    void setEnabled(bool enabled);

signals:
    void runningChanged(bool running);
    void currentPhaseChanged(MatchPhase phase);
    void timeRemainingChanged(int seconds);
    void autonomousTimeChanged(int seconds);
    void teleopTimeChanged(int seconds);
    void endgameTimeChanged(int seconds);
    void autoStartEnabledChanged(bool enabled);
    void matchStarted();
    void matchStopped();
    void matchPaused();
    void matchResumed();
    void phaseChanged(MatchPhase phase);
    void robotModeChangeRequested(int mode);
    void robotDisableRequested();

private slots:
    void updateMatch();

private:
    void transitionToPhase(MatchPhase phase);
    void updateTimeRemaining();
    QString phaseToString(MatchPhase phase) const;

    std::shared_ptr<Logger> m_logger;
    std::unique_ptr<QTimer> m_matchTimer;

    // Match state
    bool m_running;
    bool m_paused;
    MatchPhase m_currentPhase;
    int m_timeRemaining;
    qint64 m_phaseStartTime;

    // Configuration
    int m_autonomousTime;
    int m_teleopTime;
    int m_endgameTime;
    bool m_autoStartEnabled;
    bool m_robotEnabled;
};

} // namespace FRCDriverStation

#else // !ENABLE_PRACTICE_MATCH

// Stub implementation when practice match is disabled
namespace FRCDriverStation {

class Logger;

class PracticeMatchManager : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    enum MatchPhase { PreMatch = 0 };
    Q_ENUM(MatchPhase)

    explicit PracticeMatchManager(std::shared_ptr<Logger>, QObject *parent = nullptr) : QObject(parent) {}
    ~PracticeMatchManager() = default;

    bool running() const { return false; }
    MatchPhase currentPhase() const { return PreMatch; }
    int timeRemaining() const { return 0; }
    int autonomousTime() const { return 15; }
    int teleopTime() const { return 135; }
    int endgameTime() const { return 30; }
    bool autoStartEnabled() const { return false; }

    void setAutonomousTime(int) {}
    void setTeleopTime(int) {}
    void setEndgameTime(int) {}
    void setAutoStartEnabled(bool) {}

    Q_INVOKABLE void startMatch() {}
    Q_INVOKABLE void stopMatch() {}
    Q_INVOKABLE void pauseMatch() {}
    Q_INVOKABLE void resumeMatch() {}
    Q_INVOKABLE void resetMatch() {}

    void loadSettings(QSettings *) {}
    void saveSettings(QSettings *) {}
    void setEnabled(bool) {}

signals:
    void runningChanged(bool);
    void currentPhaseChanged(MatchPhase);
    void timeRemainingChanged(int);
    void autonomousTimeChanged(int);
    void teleopTimeChanged(int);
    void endgameTimeChanged(int);
    void autoStartEnabledChanged(bool);
    void matchStarted();
    void matchStopped();
    void matchPaused();
    void matchResumed();
    void phaseChanged(MatchPhase);
    void robotModeChangeRequested(int);
    void robotDisableRequested();
};

} // namespace FRCDriverStation

#endif // ENABLE_PRACTICE_MATCH

#endif // PRACTICE_MATCH_MANAGER_H
