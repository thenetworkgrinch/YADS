# Yet Another Driver Station

A modern, cross-platform FRC Driver Station application built with Qt/QML and CMake, designed to provide teams with a reliable and feature-rich interface for controlling their robots during competitions and practice sessions.

![Build Status](https://github.com/thenetworkgrinch/yet-another-driver-station/workflows/Build%20and%20Release%20Yet%20Another%20Driver%20Station/badge.svg)
![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)

## Features

### Core Functionality
- **Robot Communication**: Full FRC protocol support with real-time status monitoring
- **Emergency Stop**: Instant robot disable with global hotkeys (Space, Enter, Ctrl+E)
- **Dual Connection Modes**: Support for both competition (FMS) and practice modes
- **Controller Support**: Automatic detection and configuration of USB game controllers
- **Team Management**: Easy team number configuration with validation

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
2. Extract to your desired location
3. Run `YetAnotherDriverStation.exe`

#### macOS
1. Download `yet-another-driver-station-macos.tar.gz`
2. Extract and copy `YetAnotherDriverStation.app` to Applications
3. Run the application (you may need to allow it in Security & Privacy settings)

#### Linux
1. Download `yet-another-driver-station-linux.tar.gz`
2. Extract to your desired location
3. Make the binary executable: `chmod +x YetAnotherDriverStation`
4. Run `./YetAnotherDriverStation`

### Building from Source

#### Prerequisites
- **Qt 6.5.0 or later** with QML and Network modules
- **CMake 3.21 or later**
- **C++17 compatible compiler**
- **Git** (for cloning dependencies)

#### Platform-specific Dependencies

**Windows:**
- Visual Studio 2019 or later with MSVC
- Windows SDK

**macOS:**
- Xcode Command Line Tools
- macOS SDK 10.15 or later

**Linux:**
- Build essentials: `sudo apt-get install build-essential cmake`
- Development libraries: `sudo apt-get install libudev-dev libgl1-mesa-dev libxkbcommon-dev libx11-dev libxtst-dev`
- Qt6 development packages: `sudo apt-get install qt6-base-dev qt6-declarative-dev qt6-tools-dev`

#### Build Steps

1. **Clone the repository:**
   ```bash
   git clone https://github.com/thenetworkgrinch/yet-another-driver-station.git
   cd yet-another-driver-station
   ```

2. **Create build directory:**
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake:**
   ```bash
   # Basic configuration
   cmake .. -DCMAKE_BUILD_TYPE=Release
   
   # Or with Ninja generator (faster builds)
   cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja
   
   # Or with specific Qt installation
   cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/qt6
   ```

4. **Build the application:**
   ```bash
   # With make
   make -j$(nproc)
   
   # Or with Ninja
   ninja
   
   # Or with CMake (cross-platform)
   cmake --build . --config Release --parallel
   ```

5. **Run the application:**
   ```bash
   ./YetAnotherDriverStation  # Linux/macOS
   .\YetAnotherDriverStation.exe  # Windows
   ```

#### CMake Configuration Options

You can customize the build with various CMake options:

```bash
# Disable specific features
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_FMS_SUPPORT=OFF \
    -DENABLE_GLASS_INTEGRATION=OFF \
    -DENABLE_GLOBAL_SHORTCUTS=OFF \
    -DENABLE_DASHBOARD_MANAGEMENT=OFF \
    -DENABLE_PRACTICE_MATCH=OFF

# Enable debug logging
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG_LOGGING=ON

# Static linking (Windows/Linux)
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC=ON
```

**Available CMake Options:**
- `ENABLE_FMS_SUPPORT` (ON/OFF): Enable Field Management System integration
- `ENABLE_GLASS_INTEGRATION` (ON/OFF): Enable Glass dashboard support
- `ENABLE_GLOBAL_SHORTCUTS` (ON/OFF): Enable system-wide hotkeys
- `ENABLE_DASHBOARD_MANAGEMENT` (ON/OFF): Enable external dashboard management
- `ENABLE_PRACTICE_MATCH` (ON/OFF): Enable practice match features
- `ENABLE_DEBUG_LOGGING` (ON/OFF): Enable verbose debug logging
- `BUILD_STATIC` (ON/OFF): Build with static linking (where possible)

#### Using the Build Scripts

For convenience, you can use the provided build scripts:

**Linux/macOS:**
```bash
chmod +x scripts/build.sh
./scripts/build.sh Release
```

**Windows (PowerShell):**
```powershell
.\scripts\build.ps1 -BuildType Release
