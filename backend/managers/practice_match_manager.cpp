#include "practice_match_manager.h"

#ifdef ENABLE_PRACTICE_MATCH

#include "../core/logger.h"
#include <QDateTime>

using namespace FRCDriverStation;

PracticeMatchManager::PracticeMatchManager(std::shared_ptr<Logger> logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_matchTimer(std::make_unique<QTimer>(this))
    , m_running(false)
    , m_paused(false)
    , m_currentPhase(PreMatch)
    , m_timeRemaining(0)
    , m_phaseStartTime(0)
    , m_autonomousTime(15)
    , m_teleopTime(135)
    , m_endgameTime(30)
    , m_autoStartEnabled(false)
    , m_robotEnabled(false)
{
    // Setup match timer (10Hz for smooth updates)
    m_matchTimer->setInterval(100);
    connect(m_matchTimer.get(), &QTimer::timeout, this, &PracticeMatchManager::updateMatch);

    m_logger->info("Practice Match Manager", "Practice match manager initialized");
}

PracticeMatchManager::~PracticeMatchManager()
{
    m_logger->info("Practice Match Manager", "Practice match manager destroyed");
}

void PracticeMatchManager::setAutonomousTime(int seconds)
{
    if (m_autonomousTime != seconds) {
        m_autonomousTime = seconds;
        emit autonomousTimeChanged(seconds);
        
        m_logger->info("Practice Match", "Autonomous time changed", QString("%1 seconds").arg(seconds));
    }
}

void PracticeMatchManager::setTeleopTime(int seconds)
{
    if (m_teleopTime != seconds) {
        m_teleopTime = seconds;
        emit teleopTimeChanged(seconds);
        
        m_logger->info("Practice Match", "Teleop time changed", QString("%1 seconds").arg(seconds));
    }
}

void PracticeMatchManager::setEndgameTime(int seconds)
{
    if (m_endgameTime != seconds) {
        m_endgameTime = seconds;
        emit endgameTimeChanged(seconds);
        
        m_logger->info("Practice Match", "Endgame time changed", QString("%1 seconds").arg(seconds));
    }
}

void PracticeMatchManager::setAutoStartEnabled(bool enabled)
{
    if (m_autoStartEnabled != enabled) {
        m_autoStartEnabled = enabled;
        emit autoStartEnabledChanged(enabled);
        
        m_logger->info("Practice Match", "Auto-start changed", enabled ? "Enabled" : "Disabled");
    }
}

void PracticeMatchManager::startMatch()
{
    if (m_running) {
        return;
    }
    
    m_running = true;
    m_paused = false;
    transitionToPhase(Autonomous);
    m_matchTimer->start();
    
    emit runningChanged(true);
    emit matchStarted();
    
    m_logger->info("Practice Match", "Practice match started");
}

void PracticeMatchManager::stopMatch()
{
    if (!m_running) {
        return;
    }
    
    m_running = false;
    m_paused = false;
    m_matchTimer->stop();
    transitionToPhase(PostMatch);
    
    emit runningChanged(false);
    emit matchStopped();
    emit robotDisableRequested();
    
    m_logger->info("Practice Match", "Practice match stopped");
}

void PracticeMatchManager::pauseMatch()
{
    if (!m_running || m_paused) {
        return;
    }
    
    m_paused = true;
    m_matchTimer->stop();
    
    emit matchPaused();
    emit robotDisableRequested();
    
    m_logger->info("Practice Match", "Practice match paused");
}

void PracticeMatchManager::resumeMatch()
{
    if (!m_running || !m_paused) {
        return;
    }
    
    m_paused = false;
    m_phaseStartTime = QDateTime::currentMSecsSinceEpoch() - ((getTotalPhaseTime() - m_timeRemaining) * 1000);
    m_matchTimer->start();
    
    emit matchResumed();
    
    m_logger->info("Practice Match", "Practice match resumed");
}

void PracticeMatchManager::resetMatch()
{
    bool wasRunning = m_running;
    
    m_running = false;
    m_paused = false;
    m_matchTimer->stop();
    transitionToPhase(PreMatch);
    
    if (wasRunning) {
        emit runningChanged(false);
        emit matchStopped();
        emit robotDisableRequested();
    }
    
    m_logger->info("Practice Match", "Practice match reset");
}

void PracticeMatchManager::loadSettings(QSettings *settings)
{
    settings->beginGroup("PracticeMatchManager");
    
    m_autonomousTime = settings->value("autonomousTime", 15).toInt();
    m_teleopTime = settings->value("teleopTime", 135).toInt();
    m_endgameTime = settings->value("endgameTime", 30).toInt();
    m_autoStartEnabled = settings->value("autoStartEnabled", false).toBool();
    
    settings->endGroup();
    
    // Emit signals to update UI
    emit autonomousTimeChanged(m_autonomousTime);
    emit teleopTimeChanged(m_teleopTime);
    emit endgameTimeChanged(m_endgameTime);
    emit autoStartEnabledChanged(m_autoStartEnabled);
    
    m_logger->debug("Practice Match Manager", "Settings loaded");
}

void PracticeMatchManager::saveSettings(QSettings *settings)
{
    settings->beginGroup("PracticeMatchManager");
    
    settings->setValue("autonomousTime", m_autonomousTime);
    settings->setValue("teleopTime", m_teleopTime);
    settings->setValue("endgameTime", m_endgameTime);
    settings->setValue("autoStartEnabled", m_autoStartEnabled);
    
    settings->endGroup();
    
    m_logger->debug("Practice Match Manager", "Settings saved");
}

void PracticeMatchManager::setEnabled(bool enabled)
{
    m_robotEnabled = enabled;
    
    // Auto-start match if enabled and robot becomes enabled
    if (enabled && m_autoStartEnabled && !m_running && m_currentPhase == PreMatch) {
        startMatch();
    }
}

void PracticeMatchManager::updateMatch()
{
    if (!m_running || m_paused) {
        return;
    }
    
    updateTimeRemaining();
    
    // Check for phase transitions
    if (m_timeRemaining <= 0) {
        switch (m_currentPhase) {
            case Autonomous:
                transitionToPhase(Teleop);
                break;
            case Teleop:
                if (m_endgameTime > 0) {
                    transitionToPhase(Endgame);
                } else {
                    stopMatch();
                }
                break;
            case Endgame:
                stopMatch();
                break;
            default:
                break;
        }
    }
}

void PracticeMatchManager::transitionToPhase(MatchPhase phase)
{
    if (m_currentPhase != phase) {
        MatchPhase oldPhase = m_currentPhase;
        m_currentPhase = phase;
        m_phaseStartTime = QDateTime::currentMSecsSinceEpoch();
        
        updateTimeRemaining();
        
        emit currentPhaseChanged(phase);
        emit phaseChanged(phase);
        
        m_logger->info("Practice Match", "Phase transition", 
                      QString("From %1 to %2").arg(phaseToString(oldPhase)).arg(phaseToString(phase)));
        
        // Request robot mode changes
        switch (phase) {
            case Autonomous:
                emit robotModeChangeRequested(1); // Autonomous mode
                break;
            case Teleop:
            case Endgame:
                emit robotModeChangeRequested(0); // Teleoperated mode
                break;
            case PreMatch:
            case PostMatch:
                emit robotDisableRequested();
                break;
        }
    }
}

void PracticeMatchManager::updateTimeRemaining()
{
    int totalPhaseTime = getTotalPhaseTime();
    
    if (totalPhaseTime <= 0) {
        m_timeRemaining = 0;
    } else {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 elapsedMs = currentTime - m_phaseStartTime;
        int elapsedSeconds = static_cast<int>(elapsedMs / 1000);
        
        m_timeRemaining = qMax(0, totalPhaseTime - elapsedSeconds);
    }
    
    emit timeRemainingChanged(m_timeRemaining);
}

int PracticeMatchManager::getTotalPhaseTime() const
{
    switch (m_currentPhase) {
        case Autonomous:
            return m_autonomousTime;
        case Teleop:
            return m_teleopTime;
        case Endgame:
            return m_endgameTime;
        default:
            return 0;
    }
}

QString PracticeMatchManager::phaseToString(MatchPhase phase) const
{
    switch (phase) {
        case PreMatch: return "Pre-Match";
        case Autonomous: return "Autonomous";
        case Teleop: return "Teleop";
        case Endgame: return "Endgame";
        case PostMatch: return "Post-Match";
        default: return "Unknown";
    }
}

#endif // ENABLE_PRACTICE_MATCH
