# Yet Another Driver Station (YADS)

A modern, cross-platform FRC Driver Station application built with Qt6 and QML, designed to provide teams with a reliable and feature-rich interface for controlling their robots during competitions and practice sessions.

![Build Status](https://github.com/thenetworkgrinch/yet-another-driver-station/workflows/Build%20and%20Release/badge.svg)
![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)
![Qt Version](https://img.shields.io/badge/Qt-6.5%2B-green.svg)

## Features

### Core Functionality
- **Robot Communication**: Full FRC protocol support with real-time status monitoring
- **Emergency Stop**: Instant robot disable with global hotkeys (Space, Enter, Ctrl+E)
- **Dual Connection Modes**: Support for both competition (FMS) and practice modes
- **Controller Support**: Automatic detection and configuration of USB game controllers
- **Team Management**: Easy team number configuration with validation (1-9999)

### Advanced Features
- **Global Shortcuts**: System-wide hotkeys that work even when the application isn't focused
- **Battery Monitoring**: Real-time battery voltage tracking with configurable alerts
- **Practice Match Timer**: Built-in match simulation with autonomous/teleop periods
- **Network Diagnostics**: Comprehensive network status and troubleshooting tools
- **Dashboard Integration**: Support for multiple robot dashboards with auto-launch
- **Logging System**: Comprehensive event logging with export capabilities

### User Interface
- **Modern QML Interface**: Clean, responsive design optimized for competition use
- **Multi-tab Layout**: Organized views for Operations, Diagnostics, Controllers, and more
- **Real-time Charts**: Visual representation of robot telemetry and network status
- **Status Indicators**: Clear visual feedback for all system states
- **Customizable Layouts**: Adaptable interface for different screen sizes and preferences

## System Requirements

### Minimum Requirements
- **Operating System**: Windows 10+, macOS 10.15+, or Linux (Ubuntu 18.04+)
- **Qt Version**: Qt 6.5.0 or later
- **CMake**: CMake 3.21 or later
- **RAM**: 4GB minimum, 8GB recommended
- **Storage**: 500MB available space
- **Network**: Ethernet adapter for robot communication

### Recommended Hardware
- **CPU**: Multi-core processor (Intel i5/AMD Ryzen 5 or better)
- **RAM**: 8GB or more
- **Display**: 1920x1080 or higher resolution
- **Network**: Gigabit Ethernet adapter
- **Controllers**: Xbox/PlayStation controllers or FRC-approved joysticks

## Installation

### Pre-built Releases
Download the latest release for your platform from the [Releases](https://github.com/thenetworkgrinch/yet-another-driver-station/releases) page.

#### Windows
1. Download `yet-another-driver-station-windows.zip`
2. Extract to your desired location (e.g., `C:\\Program Files\\YADS\\`)
3. Run `YetAnotherDriverStation.exe`
4. If Windows Defender SmartScreen appears, click "More info" then "Run anyway"

#### macOS
1. Download `yet-another-driver-station-macos.tar.gz`
2. Extract and copy `YetAnotherDriverStation.app` to Applications folder
3. Right-click the app and select "Open" (required for first launch due to Gatekeeper)
4. Grant necessary permissions when prompted

#### Linux
1. Download `yet-another-driver-station-linux.tar.gz`
2. Extract to your desired location (e.g., `/opt/yads/` or `~/Applications/`)
3. Make the binary executable: `chmod +x YetAnotherDriverStation`
4. Install required system libraries if missing:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libudev1 libgl1-mesa-glx libxkbcommon0 libx11-6 libxtst6
   
   # Fedora/RHEL
   sudo dnf install libudev mesa-libGL libxkbcommon libX11 libXtst
   ```
5. Run `./YetAnotherDriverStation`

## Building from Source

### Prerequisites

#### Required Software
- **Qt 6.5.0 or later** - Download from [qt.io/download](https://www.qt.io/download)
- **CMake 3.21 or later** - Download from [cmake.org](https://cmake.org/download/)
- **Git** - For cloning the repository and dependencies
- **C++17 compatible compiler**

#### Qt Installation
You must install Qt manually from the official installer:

1. **Download Qt Online Installer** from [qt.io/download](https://www.qt.io/download)
2. **Create a Qt Account** (free for open source development)
3. **Install Qt 6.5.3 or later** with the following components:
   - **Qt Quick Controls 2** (required for QML UI)
   - **Qt Multimedia** (required for audio alerts)
   - **Qt Network Authorization** (required for FMS communication)
   - **CMake integration** (recommended)
   - **Qt Creator** (optional, but recommended for development)

#### Platform-specific Dependencies

**Windows:**
- **Visual Studio 2019 or later** with MSVC compiler
- **Windows SDK 10.0.18362 or later**
- **Git for Windows** (includes Git Bash)

**macOS:**
- **Xcode Command Line Tools**: `xcode-select --install`
- **macOS SDK 10.15 or later** (included with Xcode)

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake git pkg-config
sudo apt-get install libudev-dev libgl1-mesa-dev libxkbcommon-dev
sudo apt-get install libx11-dev libxtst-dev libxrandr-dev libxinerama-dev
sudo apt-get install libxcursor-dev libxi-dev libxss-dev
```

**Linux (Fedora/RHEL):**
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git pkg-config
sudo dnf install libudev-devel mesa-libGL-devel libxkbcommon-devel
sudo dnf install libX11-devel libXtst-devel libXrandr-devel libXinerama-devel
sudo dnf install libXcursor-devel libXi-devel libXScrnSaver-devel
```

### Build Process

#### Quick Build (Recommended)
Use the provided build scripts for the easiest setup:

**Linux/macOS:**
```bash
git clone https://github.com/thenetworkgrinch/yet-another-driver-station.git
cd yet-another-driver-station
chmod +x scripts/build.sh
./scripts/build.sh
```

**Windows (PowerShell as Administrator):**
```powershell
git clone https://github.com/thenetworkgrinch/yet-another-driver-station.git
cd yet-another-driver-station
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
.\\scripts\\build.ps1
```

#### Manual Build Process

#### Step 1: Clone Repository
```bash
git clone https://github.com/thenetworkgrinch/yet-another-driver-station.git
cd yet-another-driver-station
```

#### Step 2: Clone Dependencies
The build system automatically clones QHotkey, but you can do it manually:
```bash
git clone --depth 1 https://github.com/Skycoder42/QHotkey.git thirdparty/QHotkey
```

#### Step 3: Configure CMake
```bash
mkdir build && cd build

# Basic Release build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Advanced configuration with all features
cmake .. -DCMAKE_BUILD_TYPE=Release \\
    -DENABLE_GLOBAL_SHORTCUTS=ON \\
    -DENABLE_FMS_SUPPORT=ON \\
    -DENABLE_GLASS_INTEGRATION=ON \\
    -DENABLE_DASHBOARD_MANAGEMENT=ON \\
    -DENABLE_PRACTICE_MATCH=ON \\
    -DENABLE_DEBUG_LOGGING=OFF

# Specify Qt installation path if needed
cmake .. -DCMAKE_BUILD_TYPE=Release \\
    -DCMAKE_PREFIX_PATH="/path/to/qt6" \\
    -DQt6_DIR="/path/to/qt6/lib/cmake/Qt6"
```

#### Step 4: Build
```bash
# Build with maximum parallel jobs
cmake --build . --config Release --parallel

# Or specify job count
cmake --build . --config Release --parallel 4

# Build specific targets
cmake --build . --target YetAnotherDriverStation --config Release
```

#### Step 5: Install (Optional)
```bash
# Install to system directories
sudo cmake --install .

# Install to custom directory
cmake --install . --prefix /opt/yet-another-driver-station
```

### CMake Configuration Options

The build system supports extensive customization through CMake variables:

#### Feature Flags
```bash
# Core features (default: ON)
-DENABLE_GLOBAL_SHORTCUTS=ON/OFF    # System-wide hotkeys
-DENABLE_FMS_SUPPORT=ON/OFF         # Field Management System
-DENABLE_PRACTICE_MATCH=ON/OFF      # Practice match timer

# Dashboard integration (default: ON)
-DENABLE_GLASS_INTEGRATION=ON/OFF   # Glass dashboard support
-DENABLE_DASHBOARD_MANAGEMENT=ON/OFF # Dashboard auto-launch

# Development features (default: OFF)
-DENABLE_DEBUG_LOGGING=ON/OFF       # Verbose debug output
-DENABLE_UNIT_TESTS=ON/OFF          # Build unit tests
-DENABLE_BENCHMARKS=ON/OFF          # Performance benchmarks

# Build options
-DBUILD_SHARED_LIBS=ON/OFF          # Build as shared library
-DBUILD_STATIC=ON/OFF               # Static linking where possible
-DUSE_SYSTEM_QHOTKEY=ON/OFF         # Use system QHotkey instead of bundled
```

#### Example Configurations

**Minimal Build (Competition-only):**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \\
    -DENABLE_GLASS_INTEGRATION=OFF \\
    -DENABLE_DEBUG_LOGGING=OFF \\
    -DENABLE_UNIT_TESTS=OFF
```

**Developer Build:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug \\
    -DENABLE_DEBUG_LOGGING=ON \\
    -DENABLE_UNIT_TESTS=ON \\
    -DENABLE_BENCHMARKS=ON
```

**Maximum Features:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \\
    -DENABLE_ALL_FEATURES=ON
```

### Build Troubleshooting

#### Common Issues

**Qt Not Found:**
```bash
# Specify Qt path explicitly
cmake .. -DCMAKE_PREFIX_PATH="/usr/lib/x86_64-linux-gnu/cmake/Qt6"

# Or set environment variable
export Qt6_DIR="/path/to/qt6/lib/cmake/Qt6"
```

**QHotkey Build Failure:**
```bash
# Clean and rebuild QHotkey
rm -rf build/thirdparty/QHotkey
cmake --build . --target clean
cmake --build . --parallel
```

**Missing Dependencies (Linux):**
```bash
# Ubuntu/Debian
sudo apt-get build-dep qt6-base-dev

# Fedora
sudo dnf builddep qt6-qtbase-devel
```

**Permission Issues (macOS):**
```bash
# Grant permissions to CMake
sudo xcode-select --install
```

## Usage

### First Launch Setup

1. **Start the Application**
   - Windows: Run `YetAnotherDriverStation.exe`
   - macOS: Launch from Applications folder
   - Linux: Run `./YetAnotherDriverStation`

2. **Initial Configuration**
   - Enter your team number (1-9999) in the settings dialog
   - Select your preferred network interface
   - Configure any connected controllers

3. **Network Setup**
   - Connect to robot via Ethernet, WiFi, or USB
   - The application automatically calculates robot IP: `10.TE.AM.2`
   - Verify connection status in the Operations tab

### Basic Operation

#### Robot Control
- **Enable Robot**: Click "Enable" button or press Space
- **Disable Robot**: Click "Disable" or press Space again
- **Emergency Stop**: Press Enter or Ctrl+E for immediate disable
- **Mode Selection**: Choose Autonomous, Teleop, or Test mode using buttons or F1-F3 keys

#### Status Monitoring
- **Robot Status**: Green = Connected, Red = Disconnected, Yellow = Issues
- **Battery Voltage**: Real-time voltage display with color-coded alerts
- **Communication**: Packet loss and latency indicators
- **FMS Status**: Competition mode connectivity (when applicable)

#### Practice Matches
1. Navigate to the Operations tab
2. Click "Start Practice Match"
3. The timer automatically cycles through:
   - Autonomous period (15 seconds)
   - Teleop period (135 seconds)
4. Audio alerts signal period transitions
5. Use "Stop" to end early or "Reset" to restart

### Advanced Features

#### Global Shortcuts
These hotkeys work system-wide, even when the application is minimized:

| Shortcut | Action | Description |
|----------|--------|-------------|
| `Space` | Toggle Enable/Disable | Primary robot control |
| `Enter` | Emergency Stop | Immediate robot disable |
| `Ctrl+E` | Emergency Stop | Alternative emergency stop |
| `F1` | Autonomous Mode | Switch to autonomous |
| `F2` | Teleop Mode | Switch to teleoperated |
| `F3` | Test Mode | Switch to test mode |
| `Ctrl+Shift+D` | Toggle Dashboard | Show/hide main dashboard |

*Note: Global shortcuts can be customized in Settings → Shortcuts*

#### Controller Management
1. **Automatic Detection**: Controllers are detected when plugged in
2. **Real-time Testing**: View live input in the Controllers tab
3. **Button Mapping**: Customize button assignments
4. **Calibration**: Adjust deadzone and sensitivity settings
5. **Multiple Controllers**: Support for up to 6 simultaneous controllers

#### Dashboard Integration
- **Auto-launch**: Automatically start dashboards with the driver station
- **Process Monitoring**: Monitor dashboard health and restart if needed
- **Multiple Dashboards**: Run several dashboards simultaneously
- **Custom Arguments**: Pass custom command-line arguments to dashboards

## Configuration

### Settings File Locations

Configuration files are stored in platform-specific locations:

- **Windows**: `%APPDATA%\\Yet Another Driver Station\\`
- **macOS**: `~/Library/Application Support/Yet Another Driver Station/`
- **Linux**: `~/.config/Yet Another Driver Station/`

### Configuration Files

#### `settings.ini` - Main Configuration
```ini
[General]
team_number=1234
theme=dark
auto_start_practice=false

[Network]
preferred_interface=Ethernet
timeout_ms=1000
auto_reconnect=true

[Audio]
enable_alerts=true
volume=75
alert_sounds=true

[Shortcuts]
enable_global=true
emergency_stop=Return
toggle_enable=Space
```

#### `dashboards.json` - Dashboard Configuration
```json
{
  "dashboards": [
    {
      "name": "SmartDashboard",
      "executable": "SmartDashboard.jar",
      "arguments": ["-jar"],
      "auto_start": true,
      "monitor": true
    },
    {
      "name": "Glass",
      "executable": "glass",
      "arguments": ["--team", "${TEAM_NUMBER}"],
      "auto_start": false,
      "monitor": false
    }
  ]
}
```

#### `controllers.json` - Controller Configuration
```json
{
  "controllers": [
    {
      "name": "Xbox Controller",
      "vendor_id": "045e",
      "product_id": "02ea",
      "button_mapping": {
        "A": 0,
        "B": 1,
        "X": 2,
        "Y": 3
      },
      "axis_mapping": {
        "left_stick_x": 0,
        "left_stick_y": 1,
        "right_stick_x": 2,
        "right_stick_y": 3
      }
    }
  ]
}
```

### Network Configuration

#### Team Number Setup
The team number determines all network addresses:
- **Robot**: `10.TE.AM.2` (e.g., team 1234 → `10.12.34.2`)
- **Driver Station**: `10.TE.AM.5`
- **Radio**: `10.TE.AM.1`

#### Connection Priorities
1. **Ethernet**: Preferred for competitions (lowest latency)
2. **WiFi**: Backup connection method
3. **USB**: Direct connection for testing (requires additional setup)

#### Firewall Configuration

**Windows Firewall:**
- Allow "Yet Another Driver Station" through Windows Defender Firewall
- Open ports: 1110, 1140, 1735 (TCP), 1130 (UDP)

**macOS Firewall:**
- Grant network access permissions when prompted
- Add exception in Security & Privacy → Firewall

**Linux Firewall (iptables/ufw):**
```bash
# UFW
sudo ufw allow 1110/tcp
sudo ufw allow 1140/tcp  
sudo ufw allow 1735/tcp
sudo ufw allow 1130/udp

# iptables
sudo iptables -A INPUT -p tcp --dport 1110 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 1140 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 1735 -j ACCEPT
sudo iptables -A INPUT -p udp --dport 1130 -j ACCEPT
```

## Troubleshooting

### Common Issues and Solutions

#### Robot Connection Issues

**Problem**: Robot not connecting despite correct setup
**Solutions**:
1. Verify team number is correct (1-9999)
2. Check network cable connections
3. Ping robot IP address: `ping 10.TE.AM.2`
4. Restart robot and driver station
5. Check firewall settings
6. Try different network interface

**Problem**: Intermittent connection drops
**Solutions**:
1. Check cable quality and connections
2. Update network adapter drivers
3. Disable power management on network adapter
4. Check for network interference (WiFi)
5. Monitor packet loss in diagnostics tab

#### Controller Issues

**Problem**: Controllers not detected
**Solutions**:
1. Unplug and reconnect controller
2. Try different USB port
3. Install/update controller drivers
4. Check Device Manager (Windows) for hardware issues
5. Test controller in other applications
6. Check USB cable quality

**Problem**: Controller input lag or errors
**Solutions**:
1. Use USB 2.0 port instead of USB 3.0
2. Avoid USB hubs, connect directly
3. Update controller firmware
4. Adjust polling rate in settings
5. Check for conflicting software

#### Global Shortcuts Not Working

**Windows**:
1. Run application as Administrator
2. Check Windows Defender Firewall exceptions
3. Disable conflicting gaming software
4. Verify no other applications using same shortcuts
5. Check Windows accessibility settings

**macOS**:
1. Grant Accessibility permissions in System Preferences → Security & Privacy
2. Add application to Input Monitoring permissions
3. Restart application after granting permissions
4. Check for conflicting system shortcuts
5. Verify Secure Keyboard Entry is disabled in Terminal

**Linux**:
1. Ensure X11 session (not Wayland)
2. Install required X11 development libraries
3. Check desktop environment shortcut conflicts
4. Verify user permissions for input devices
5. Test with different window manager

#### Build Issues

**CMake Configuration Errors**:
```bash
# Clear CMake cache
rm -rf build
mkdir build && cd build

# Specify Qt path explicitly
cmake .. -DCMAKE_PREFIX_PATH="/path/to/qt6"

# Check Qt installation
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=ON
```

**Compilation Errors**:
```bash
# Update compiler
# Ubuntu: sudo apt update && sudo apt upgrade gcc g++
# macOS: xcode-select --install
# Windows: Update Visual Studio

# Check C++17 support
gcc --version  # Should be 7.0+
clang --version  # Should be 5.0+
```

**Linking Errors**:
```bash
# Clear build directory
rm -rf build

# Rebuild dependencies
git submodule update --init --recursive
cmake --build . --target clean
cmake --build . --parallel
```

**QHotkey Issues**:
```bash
# Manual QHotkey clone
rm -rf thirdparty/QHotkey
git clone https://github.com/Skycoder42/QHotkey.git thirdparty/QHotkey

# Build QHotkey separately
cd thirdparty/QHotkey
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Getting Help

#### Support Channels
- **GitHub Issues**: [Repository Issues Page](https://github.com/thenetworkgrinch/yet-another-driver-station/issues)
- **GitHub Discussions**: [Community Discussions](https://github.com/thenetworkgrinch/yet-another-driver-station/discussions)
- **Discord Server**: [Community Discord](https://discord.gg/your-server) (link in repository)
- **Email Support**: support@your-domain.com

## Development

### Project Structure
```
yet-another-driver-station/
├── CMakeLists.txt              # Main CMake configuration
├── main.cpp                    # Application entry point
├── main.qml                    # Main QML interface
├── qml.qrc                     # QML resources
│
├── backend/                    # C++ backend implementation
│   ├── core/                   # Core utilities and services
│   ├── managers/               # High-level system managers
│   ├── robot/                  # Robot communication subsystem
│   └── controllers/            # Game controller support
│
├── qml/                        # QML user interface components
├── dashboards/                 # Dashboard configurations
├── scripts/                    # Build and utility scripts
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
