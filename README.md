# Yet Another Driver Station

A modern, feature-rich FRC Driver Station application built with Qt/QML, designed to provide teams with a reliable and intuitive interface for controlling their robots during competitions and practice sessions.

![Build Status](https://github.com/thenetworkgrinch/yet-another-driver-station/workflows/Build%20and%20Release%20Yet%20Another%20Driver%20Station/badge.svg)
![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)

## Features

### Core Functionality
- **Dual Connection Modes**: Connect via traditional team numbers or direct IP addresses
- **Robot Communication**: Full FRC protocol support with automatic robot discovery
- **Real-time Status**: Live robot status, battery voltage, and connection monitoring
- **Emergency Controls**: Global emergency stop and robot disable/enable functionality
- **Match Logging**: Comprehensive logging system with match-based organization

### Advanced Features
- **Global Shortcuts**: System-wide hotkeys that work even when the application isn't focused
- **Dashboard Management**: Support for multiple dashboards (Glass, Shuffleboard, SmartDashboard)
- **Practice Match Support**: Built-in practice match timer and management
- **Network Diagnostics**: Advanced network monitoring and troubleshooting tools
- **Crash Recovery**: Automatic crash detection and recovery system

### User Interface
- **Modern Design**: Clean, intuitive interface built with Qt/QML
- **Dual Connection Modes**: Toggle between team number and IP address input
- **Dark/Light Theme**: Professional themes optimized for competition environments
- **Real-time Updates**: Live status indicators and smooth animations
- **Accessibility**: Full keyboard navigation and screen reader support

### Platform Support
- **Windows**: Full support for Windows 10/11 (x64)
- **macOS**: Native support for Apple Silicon (ARM64) Macs
- **Linux**: Compatible with major distributions (Ubuntu, Fedora, etc.)

## Quick Start

### Prerequisites
- Qt 6.5.0 or later
- C++17 compatible compiler
- Git (for cloning QHotkey dependency)
- Platform-specific dependencies (see [Building](#building))

### Installation

#### Download Pre-built Binaries
1. Go to the [Releases](https://github.com/thenetworkgrinch/yet-another-driver-station/releases) page
2. Download the appropriate package for your platform
3. Extract and run the application

#### Build from Source
```bash
git clone https://github.com/thenetworkgrinch/yet-another-driver-station.git
cd yet-another-driver-station

# Clone QHotkey dependency
mkdir -p thirdparty
cd thirdparty
git clone https://github.com/Skycoder42/QHotkey.git
cd QHotkey
git checkout 1.5.0
cd ../..

# Build
qmake YetAnotherDriverStation.pro
make
```

### First Run
1. Launch the application
2. Choose your connection mode:
   - **Team Number**: Enter your 4-digit team number (1-9999)
   - **IP Address**: Enter the robot's IP address directly
3. Click "Connect" to establish communication with your robot
4. The application will automatically save your settings for future sessions

## Configuration

### Connection Modes

#### Team Number Mode
- Traditional FRC connection using team numbers
- Automatically tries multiple robot addresses:
  - `10.TE.AM.2` (standard competition network)
  - `172.22.11.2` (USB connection)
  - `192.168.1.2` (Ethernet connection)
- Supports mDNS discovery (`roboRIO-TEAM-FRC.local`)

#### IP Address Mode
- Direct connection to a specific IP address
- Useful for custom network configurations
- Supports any valid IPv4 address
- Real-time IP address validation

### Global Shortcuts
Global shortcuts work system-wide, even when the application isn't focused:

| Shortcut | Action | Description |
|----------|--------|-------------|
| `Space` | Emergency Stop | Immediately stops the robot |
| `Enter` | Disable Robot | Safely disables the robot |
| `Ctrl+E` | Enable Robot | Enables the robot (if not e-stopped) |

Global shortcuts can be disabled in the settings if needed.

### Dashboard Management
The application supports multiple dashboard applications:
- **Driver Station**: Built-in dashboard with diagnostics
- **Glass**: WPILib's modern dashboard (auto-detected)
- **Shuffleboard**: Official FRC dashboard
- **SmartDashboard**: Legacy FRC dashboard
- **Custom**: Add your own dashboard applications

## Building

### Dependencies

#### All Platforms
- Qt 6.5.0+ with QML and Network modules
- Git (for QHotkey dependency)

#### Windows
- Visual Studio 2019 or later
- Windows SDK

#### macOS
- Xcode Command Line Tools
- macOS 11.0+ (for ARM64 support)

#### Linux
- GCC 9+ or Clang 10+
- libudev-dev
- libgl1-mesa-dev
- libxkbcommon-dev
- libx11-dev
- libxtst-dev

### Build Instructions

#### Standard Build
```bash
# Clone the repository
git clone https://github.com/thenetworkgrinch/yet-another-driver-station.git
cd yet-another-driver-station

# Clone QHotkey dependency
mkdir -p thirdparty
cd thirdparty
git clone https://github.com/Skycoder42/QHotkey.git
cd QHotkey
git checkout 1.5.0
cd ../..

# Build QHotkey first (platform-specific project file will be created automatically)
cd thirdparty/QHotkey
qmake QHotkey.pro CONFIG+=release
make
cd ../..

# Build main application
qmake YetAnotherDriverStation.pro CONFIG+=release
make
```

#### Feature Flags
You can disable specific features during build:

```bash
# Disable global shortcuts
qmake CONFIG+=no_global_shortcuts

# Disable FMS support
qmake CONFIG+=no_fms_support

# Disable dashboard management
qmake CONFIG+=no_dashboard_management

# Disable multiple features
qmake CONFIG+=no_global_shortcuts CONFIG+=no_fms_support
```

Available feature flags:
- `no_global_shortcuts`: Disables global hotkey functionality
- `no_fms_support`: Removes FMS integration
- `no_dashboard_management`: Disables external dashboard support
- `no_practice_match`: Removes practice match features
- `no_glass_integration`: Disables Glass dashboard integration

### QHotkey Integration

This project uses QHotkey for global shortcut functionality. The dependency is automatically managed:

1. **Automatic Detection**: The build system checks for QHotkey in `thirdparty/QHotkey`
2. **Platform-Specific Building**: Creates appropriate project files for each platform
3. **Graceful Degradation**: If QHotkey is not found, global shortcuts are automatically disabled
4. **Static Linking**: QHotkey is built as a static library and linked into the main application

#### Manual QHotkey Setup
If you need to set up QHotkey manually:

```bash
mkdir -p thirdparty
cd thirdparty
git clone https://github.com/Skycoder42/QHotkey.git
cd QHotkey
git checkout 1.5.0

# The build system will automatically create the appropriate .pro file
# and build QHotkey when you build the main application
```

## Usage

### Basic Operation
1. **Choose Connection Mode**: Select either Team Number or IP Address mode
2. **Set Target**: Enter your team number (1-9999) or robot IP address
3. **Connect**: Click the Connect button or use `Ctrl+R`
4. **Monitor Status**: Watch the connection and robot status indicators
5. **Control Robot**: Use the enable/disable buttons or global shortcuts
6. **Emergency Stop**: Press `Space` or click the E-Stop button if needed

### Connection Modes

#### Team Number Mode
- Enter your 4-digit FRC team number
- Application automatically tries multiple connection methods
- Supports standard FRC network configurations
- Ideal for competition and standard practice environments

#### IP Address Mode
- Enter the robot's IP address directly (e.g., `10.0.0.2`)
- Real-time validation ensures proper IP format
- Useful for custom network setups or development environments
- Supports any valid IPv4 address

### Advanced Features

#### Match Logging
- Logs are automatically organized by match type and number
- Practice matches are logged separately
- All logs include timestamps, robot status, and network events
- Crash recovery ensures no log data is lost

#### Network Diagnostics
- Real-time ping latency monitoring
- Packet loss statistics
- Connection quality indicators
- Network interface information

#### Dashboard Integration
- Launch external dashboards directly from the application
- Monitor dashboard process status
- Automatic path detection for common dashboards
- Custom dashboard configuration support

## Troubleshooting

### Common Issues

#### Robot Not Found
- Verify team number is correct (1-9999) or IP address is valid
- Check network connection (Ethernet, WiFi, or USB)
- Ensure robot is powered on and running code
- Try switching between connection modes
- In team number mode, the app tries multiple addresses automatically

#### Global Shortcuts Not Working
- Check if global shortcuts are enabled in settings
- Verify no other applications are using the same shortcuts
- On Linux, ensure proper permissions for input devices
- Try running as administrator (Windows) or with sudo (Linux)
- If QHotkey failed to build, global shortcuts will be automatically disabled

#### Dashboard Won't Launch
- Verify dashboard path is correctly configured
- Check that the dashboard executable exists and is accessible
- Ensure proper permissions to execute the dashboard
- Check dashboard-specific requirements and dependencies

#### Build Issues
- Ensure Qt 6.5.0+ is installed with QML and Network modules
- Verify QHotkey was cloned correctly in `thirdparty/QHotkey`
- Check that all platform-specific dependencies are installed
- Try building with feature flags disabled if encountering issues

### Log Files
Log files are stored in:
- **Windows**: `%APPDATA%/Yet Another Driver Station/logs/`
- **macOS**: `~/Library/Application Support/Yet Another Driver Station/logs/`
- **Linux**: `~/.local/share/Yet Another Driver Station/logs/`

## Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Development Setup
1. Fork the repository
2. Clone QHotkey dependency: `git clone https://github.com/Skycoder42/QHotkey.git thirdparty/QHotkey`
3. Create a feature branch
4. Make your changes
5. Test on multiple platforms if possible
6. Submit a pull request

### Code Style
- Follow Qt/C++ coding conventions
- Use meaningful variable and function names
- Add documentation for public APIs
- Include unit tests for new features
- Test both connection modes when making networking changes

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **WPILib Team**: For the FRC protocol specifications and reference implementations
- **Qt Project**: For the excellent Qt framework
- **FRC Community**: For feedback, testing, and contributions
- **Skycoder42**: For the QHotkey library enabling global shortcuts

## Support

- **Issues**: Report bugs and request features on [GitHub Issues](https://github.com/thenetworkgrinch/yet-another-driver-station/issues)
- **Discussions**: Join the conversation on [GitHub Discussions](https://github.com/thenetworkgrinch/yet-another-driver-station/discussions)
- **Discord**: Join our community Discord server (link in repository)

---

**Yet Another Driver Station** - Because every team deserves reliable, modern driver station software with flexible connection options.
