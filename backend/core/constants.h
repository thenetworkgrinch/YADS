#pragma once

#include <QString>
#include <QColor>

namespace Constants {
    // Application information
    constexpr const char* APPLICATION_NAME = "Yet Another Driver Station";
    constexpr const char* APPLICATION_VERSION = "2025.1.0";
    constexpr const char* ORGANIZATION_NAME = "FRC Community";
    constexpr const char* ORGANIZATION_DOMAIN = "frc-community.org";
    
    // Network configuration
    constexpr int DEFAULT_ROBOT_PORT = 1110;
    constexpr int DEFAULT_FMS_PORT = 1120;
    constexpr int DEFAULT_DASHBOARD_PORT = 1735;
    constexpr int DEFAULT_GLASS_PORT = 3300;
    constexpr int DEFAULT_NETWORKTABLES_PORT = 1735;
    
    constexpr int COMMUNICATION_TIMEOUT_MS = 1000;
    constexpr int HEARTBEAT_INTERVAL_MS = 100;
    constexpr int CONNECTION_RETRY_INTERVAL_MS = 5000;
    
    // Team number constraints
    constexpr int MIN_TEAM_NUMBER = 1;
    constexpr int MAX_TEAM_NUMBER = 9999;
    
    // Robot modes
    enum class RobotMode {
        Disabled = 0,
        Autonomous = 1,
        Teleop = 2,
        Test = 3
    };
    
    // Robot states
    enum class RobotState {
        Disconnected = 0,
        Connected = 1,
        Enabled = 2,
        EmergencyStop = 3
    };
    
    // Battery voltage thresholds
    constexpr double BATTERY_VOLTAGE_CRITICAL = 10.5;
    constexpr double BATTERY_VOLTAGE_WARNING = 11.5;
    constexpr double BATTERY_VOLTAGE_GOOD = 12.0;
    constexpr double BATTERY_VOLTAGE_MAX = 13.0;
    
    // Practice match timing (in seconds)
    constexpr int PRACTICE_MATCH_AUTO_DURATION = 15;
    constexpr int PRACTICE_MATCH_TELEOP_DURATION = 135;
    constexpr int PRACTICE_MATCH_TRANSITION_DURATION = 5;
    
    // UI Colors
    namespace Colors {
        const QColor ROBOT_CONNECTED = QColor("#4CAF50");      // Green
        const QColor ROBOT_DISCONNECTED = QColor("#F44336");   // Red
        const QColor ROBOT_WARNING = QColor("#FF9800");        // Orange
        const QColor ROBOT_ENABLED = QColor("#2196F3");        // Blue
        
        const QColor BATTERY_GOOD = QColor("#4CAF50");         // Green
        const QColor BATTERY_WARNING = QColor("#FF9800");      // Orange
        const QColor BATTERY_CRITICAL = QColor("#F44336");     // Red
        
        const QColor FMS_CONNECTED = QColor("#4CAF50");        // Green
        const QColor FMS_DISCONNECTED = QColor("#9E9E9E");     // Gray
        
        const QColor BACKGROUND_DARK = QColor("#2B2B2B");
        const QColor BACKGROUND_LIGHT = QColor("#FFFFFF");
        const QColor SURFACE_DARK = QColor("#3C3C3C");
        const QColor SURFACE_LIGHT = QColor("#F5F5F5");
    }
    
    // File paths
    namespace Paths {
        const QString CONFIG_DIR = "config";
        const QString LOGS_DIR = "logs";
        const QString DASHBOARDS_DIR = "dashboards";
        const QString SETTINGS_FILE = "settings.ini";
        const QString CONTROLLERS_FILE = "controllers.json";
        const QString DASHBOARDS_FILE = "dashboards.json";
    }
    
    // Network utilities
    class NetworkUtils {
    public:
        static QString calculateRobotIP(int teamNumber) {
            if (teamNumber < MIN_TEAM_NUMBER || teamNumber > MAX_TEAM_NUMBER) {
                return QString();
            }
            
            int firstOctet = teamNumber / 100;
            int secondOctet = teamNumber % 100;
            
            return QString("10.%1.%2.2").arg(firstOctet).arg(secondOctet);
        }
        
        static QString calculateDriverStationIP(int teamNumber) {
            if (teamNumber < MIN_TEAM_NUMBER || teamNumber > MAX_TEAM_NUMBER) {
                return QString();
            }
            
            int firstOctet = teamNumber / 100;
            int secondOctet = teamNumber % 100;
            
            return QString("10.%1.%2.5").arg(firstOctet).arg(secondOctet);
        }
        
        static QString calculateRadioIP(int teamNumber) {
            if (teamNumber < MIN_TEAM_NUMBER || teamNumber > MAX_TEAM_NUMBER) {
                return QString();
            }
            
            int firstOctet = teamNumber / 100;
            int secondOctet = teamNumber % 100;
            
            return QString("10.%1.%2.1").arg(firstOctet).arg(secondOctet);
        }
    };
    
    // Logging categories
    namespace LogCategories {
        constexpr const char* ROBOT_COMM = "robot.communication";
        constexpr const char* NETWORK = "network";
        constexpr const char* CONTROLLERS = "controllers";
        constexpr const char* FMS = "fms";
        constexpr const char* DASHBOARDS = "dashboards";
        constexpr const char* BATTERY = "battery";
        constexpr const char* PRACTICE_MATCH = "practice.match";
        constexpr const char* GLOBAL_SHORTCUTS = "shortcuts";
    }
}
