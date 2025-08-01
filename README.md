# Yet Another Driver Station (YADS)

[![Build Status](https://github.com/thenetworkgrinch/yet-another-driver-station/workflows/Build%20and%20Release/badge.svg)](https://github.com/thenetworkgrinch/yet-another-driver-station/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Qt Version](https://img.shields.io/badge/Qt-6.5%2B-green.svg)](https://www.qt.io/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-blue.svg)](#supported-platforms)

**A modern, cross-platform FRC Driver Station built with Qt6 and QML**

Yet Another Driver Station (YADS) is a feature-rich, open-source alternative to the official FRC Driver Station. Built with modern C++17 and Qt6, it provides reliable robot communication, comprehensive diagnostics, and an intuitive user interface for FIRST Robotics Competition teams.

## Features

### Core Functionality
- ✅ **Full FRC Protocol Support** - Complete implementation of the FRC communication protocol
- ✅ **Cross-Platform** - Native support for Windows, macOS, and Linux
- ✅ **Modern UI** - Responsive QML interface with dark/light themes
- ✅ **Global Shortcuts** - System-wide hotkeys for robot control
- ✅ **Multi-Controller Support** - Xbox, PlayStation, and generic HID controllers
- ✅ **Practice Match Timer** - Official FRC timing with autonomous and teleop periods
- ✅ **Real-time Diagnostics** - Network latency, packet loss, and connection monitoring
- ✅ **Battery Monitoring** - Real-time voltage display with configurable alerts
- ✅ **Dashboard Integration** - Auto-launch and manage SmartDashboard, Shuffleboard, and Glass

### Advanced Features
- 🔧 **Network Diagnostics** - Built-in ping, bandwidth testing, and port scanning
- 📊 **Telemetry Visualization** - Real-time charts and graphs for robot data
- 🎮 **Controller Calibration** - Advanced joystick deadzone and sensitivity settings
- 📝 **Comprehensive Logging** - Detailed event logging with automatic rotation
- 🔒 **Competition Mode** - FMS integration with official match timing
- 🎯 **Emergency Stop** - Multiple emergency stop methods with audio/visual alerts
- 📱 **Responsive Design** - Adapts to different screen sizes and orientations
- 🌐 **Multi-Language Ready** - Internationalization framework (English included)

## Global Shortcuts

Global shortcuts work system-wide, even when the application isn't focused:

### Available Shortcuts
- **Space**: Toggle robot enable/disable
- **Enter**: Enable/disable the currently selected mode (Autonomous, Teleop, or Test)

*Note: Global shortcuts can be disabled at build time with `-DENABLE_GLOBAL_SHORTCUTS=OFF`*

### Platform-Specific Behavior

#### Windows
- Uses Windows API for global hook registration
- May require elevation for some system-level shortcuts
- Respects Windows accessibility settings

#### macOS
- Uses Carbon framework for global event monitoring
- Requires Accessibility permissions (granted on first use)
- Integrates with macOS shortcut system preferences

#### Linux
- Uses X11 for global key capture
- Requires X11 session (limited Wayland support)
- May conflict with desktop environment shortcuts

## System Requirements

### Minimum Requirements
- **OS**: Windows 10 (1903+), macOS 10.15+, or Linux (Ubuntu 20.04+/equivalent)
- **CPU**: Dual-core 2.0 GHz processor
- **RAM**: 4 GB system memory
- **Storage**: 500 MB available disk space
- **Network**: Ethernet adapter (WiFi supported but not recommended for competitions)
- **Graphics**: OpenGL 3.3 compatible graphics card

### Recommended Requirements
- **OS**: Windows 11, macOS 12+, or Linux (Ubuntu 22.04+/equivalent)
- **CPU**: Quad-core 3.0 GHz processor or better
- **RAM**: 8 GB system memory or more
- **Storage**: 2 GB available disk space (for logs and recordings)
- **Network**: Gigabit Ethernet adapter
- **Graphics**: Dedicated graphics card with OpenGL 4.0+ support
- **Controllers**: Xbox or PlayStation controllers for optimal compatibility

### Development Requirements
- **Qt 6.5.0+** with QML, Multimedia, and Charts modules
- **CMake 3.21+** for building
- **C++17 compatible compiler** (GCC 9+, Clang 10+, MSVC 2019+)
- **Git** for source control and dependency management

## Installation

**Important**: This project must be built from source. No package manager installations are available.

### Option 1: Pre-built Releases (Recommended)

Download the latest release for your platform from the [GitHub Releases](https://github.com/thenetworkgrinch/yet-another-driver-station/releases) page:

**Windows:**
1. Download `YetAnotherDriverStation-Windows.zip`
2. Extract to your desired location (e.g., `C:\Program Files\YetAnotherDriverStation`)
3. Run `YetAnotherDriverStation.exe`

**macOS:**
1. Download `YetAnotherDriverStation-macOS.tar.gz`
2. Extract: `tar -xzf YetAnotherDriverStation-macOS.tar.gz`
3. Copy `YetAnotherDriverStation.app` to Applications folder
4. Launch from Applications folder
5. Grant necessary permissions when prompted

**Linux:**
1. Download `YetAnotherDriverStation-Linux.AppImage`
2. Make executable: `chmod +x YetAnotherDriverStation-Linux.AppImage`
3. Run: `./YetAnotherDriverStation-Linux.AppImage`

### Option 2: Build from Source

See the [Building from Source](#building-from-source) section below for detailed instructions.

## Building from Source

### Prerequisites

#### Qt 6.5.0+ Installation

**Windows:**
1. Download Qt Online Installer from [qt.io/download](https://www.qt.io/download-qt-installer)
2. Install Qt 6.5.3 or later with these components:
   - Qt Quick Controls 2
   - Qt Multimedia
   - Qt Charts
   - CMake integration
   - Debugging Tools

**macOS:**
1. Download Qt Online Installer from [qt.io/download](https://www.qt.io/download-qt-installer)
2. Install Qt 6.5.3 or later with required components

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install qt6-base-dev qt6-declarative-dev qt6-multimedia-dev \
                 qt6-charts-dev cmake build-essential git pkg-config \
                 libudev-dev libgl1-mesa-dev
```

**Linux (Fedora/RHEL):**
```bash
sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtmultimedia-devel \
                 qt6-qtcharts-devel cmake gcc-c++ git pkgconfig \
                 systemd-devel mesa-libGL-devel
```

#### Additional Tools
- **Git**: For cloning repository and dependencies
- **CMake 3.21+**: Modern build system
- **C++17 Compiler**: Platform-appropriate compiler

### Quick Build (Automated Scripts)

The fastest way to build YADS is using the provided build scripts:

#### Linux/macOS
```bash
git clone https://github.com/thenetworkgrinch/yet-another-driver-station.git
cd yet-another-driver-station
chmod +x scripts/build.sh
./scripts/build.sh
```

#### Windows (PowerShell)
```powershell
git clone https://github.com/thenetworkgrinch/yet-another-driver-station.git
cd yet-another-driver-station
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
.\scripts\build.ps1
```

### Manual Build Process

#### Step 1: Clone Repository
```bash
git clone https://github.com/thenetworkgrinch/yet-another-driver-station.git
cd yet-another-driver-station
```

#### Step 2: Clone QHotkey Dependency
```bash
git clone --depth 1 https://github.com/Skycoder42/QHotkey.git thirdparty/QHotkey
```

#### Step 3: Configure CMake
```bash
mkdir build
cd build

# Basic Release build
cmake .. -DCMAKE_BUILD_TYPE=Release

# With specific Qt installation path
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/path/to/Qt/6.5.3/gcc_64"
```

#### Step 4: Build the Application
```bash
# Cross-platform build command
cmake --build . --config Release --parallel
```

#### Step 5: Run the Application
```bash
# Linux/macOS
./YetAnotherDriverStation

# Windows
.\Release\YetAnotherDriverStation.exe
```

### Build Configuration Options

Customize your build with CMake options:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_GLOBAL_SHORTCUTS=ON \
    -DENABLE_FMS_SUPPORT=ON \
    -DENABLE_GLASS_INTEGRATION=ON \
    -DENABLE_DASHBOARD_MANAGEMENT=ON \
    -DENABLE_PRACTICE_MATCH=ON \
    -DENABLE_DEBUG_LOGGING=OFF
```

**Available Options:**
- `ENABLE_GLOBAL_SHORTCUTS` (ON/OFF): System-wide hotkeys (default: ON)
- `ENABLE_FMS_SUPPORT` (ON/OFF): Field Management System integration (default: ON)
- `ENABLE_GLASS_INTEGRATION` (ON/OFF): Glass dashboard support (default: ON)
- `ENABLE_DASHBOARD_MANAGEMENT` (ON/OFF): External dashboard management (default: ON)
- `ENABLE_PRACTICE_MATCH` (ON/OFF): Practice match timer functionality (default: ON)
- `ENABLE_DEBUG_LOGGING` (ON/OFF): Verbose debug output (default: OFF)

## Usage

### First Time Setup

1. **Launch the Application**
   - Windows: Double-click `YetAnotherDriverStation.exe`
   - macOS: Open `YetAnotherDriverStation.app` from Applications
   - Linux: Run `./YetAnotherDriverStation-Linux.AppImage`

2. **Configure Team Settings**
   - Click the Settings icon (gear) in the top-right corner
   - Enter your team number (1-9999)
   - The robot IP address will be automatically calculated as `10.TE.AM.2`
   - Click "Save" to apply settings

3. **Network Connection**
   - Connect to your robot via Ethernet cable (recommended for competitions)
   - Or connect to the robot's WiFi network (for practice)
   - The application will automatically attempt to connect

4. **Controller Setup**
   - Plug in your Xbox, PlayStation, or other HID-compatible controllers
   - Go to the "Controllers" tab to verify detection
   - Controllers are automatically mapped but can be customized

### Basic Operation

#### Robot Control
- **Enable Robot**: Click the large "Enable" button or press `Space`
- **Disable Robot**: Click "Disable" or press `Space` again
- **Enable/Disable Current Mode**: Press `Enter` to enable/disable the currently selected mode
- **Mode Selection**: Choose between Autonomous, Teleop, or Test mode using the mode buttons

#### Status Monitoring
- **Robot Status**: Green = Connected and enabled, Red = Disconnected or disabled
- **Battery Voltage**: Displayed in real-time with color-coded alerts
- **Network Latency**: Round-trip time to robot displayed in milliseconds
- **FMS Status**: Shows connection status during official matches

### Practice Matches

The built-in practice match timer simulates official FRC match timing:

1. **Start Practice Match**:
   - Go to the "Operations" tab
   - Click "Start Practice Match"
   - The robot will be automatically disabled

2. **Match Phases**:
   - **Autonomous**: 15 seconds (robot can be enabled)
   - **Transition**: 5 seconds (robot automatically disabled)
   - **Teleop**: 135 seconds (robot can be enabled)
   - **End**: Robot automatically disabled

3. **Manual Control**:
   - You can manually enable/disable the robot during each phase
   - Emergency stop works at any time
   - Match can be stopped early with "Stop Match" button

## Troubleshooting

### Common Issues and Solutions

#### Robot Connection Issues

**Problem**: Robot status shows "Disconnected" or "No Communication"

**Solutions**:
1. Verify team number is correct (1-9999)
2. Check network cable connections
3. Ping robot IP address: `ping 10.TE.AM.2`
4. Restart robot and driver station
5. Check firewall settings
6. Try different network interface

#### Global Shortcuts Not Working

**Windows**:
1. Run application as Administrator
2. Check Windows Defender Firewall exceptions
3. Disable conflicting gaming software
4. Verify no other applications using same shortcuts

**macOS**:
1. Grant Accessibility permissions in System Preferences → Security & Privacy
2. Add application to Input Monitoring permissions
3. Restart application after granting permissions

**Linux**:
1. Ensure X11 session (not Wayland)
2. Install required X11 development libraries
3. Check desktop environment shortcut conflicts

#### Build Issues

**CMake Configuration Errors**:
```bash
# Clear CMake cache
rm -rf build
mkdir build && cd build

# Specify Qt path explicitly
cmake .. -DCMAKE_PREFIX_PATH="/path/to/qt6"
```

**QHotkey Issues**:
```bash
# Manual QHotkey clone
rm -rf thirdparty/QHotkey
git clone https://github.com/Skycoder42/QHotkey.git thirdparty/QHotkey
```

### Debug Mode

For detailed troubleshooting, build with debug mode:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG_LOGGING=ON
cmake --build . --config Debug
```

## Development

### Project Structure
```
yet-another-driver-station/
├── CMakeLists.txt              # Main CMake configuration
├── main.cpp                    # Application entry point
├── backend/                    # C++ backend implementation
│   ├── core/                  # Core utilities and constants
│   ├── managers/              # System managers
│   ├── robot/                 # Robot communication subsystem
│   └── controllers/           # Game controller support
├── qml/                       # QML user interface components
├── dashboards/                # Dashboard configurations
├── scripts/                   # Build and utility scripts
├── thirdparty/                # Third-party dependencies (auto-managed)
└── .github/                   # GitHub integration
```

### Contributing
1. Fork the repository on GitHub
2. Create a feature branch: `git checkout -b feature/your-feature-name`
3. Make your changes and test thoroughly
4. Add tests if applicable
5. Update documentation if needed
6. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **FIRST Robotics Competition** for the protocol specifications and community
- **Qt Project** for the excellent cross-platform framework and QML
- **QHotkey** library by Skycoder42 for global shortcut support
- **CMake** community for the modern build system
- **FRC Community** for feedback, testing, and contributions

---

**Built with ❤️ for the FRC Community**

*Modern CMake • Cross-Platform • Open Source*

*This project must be built from source - No package manager required*
