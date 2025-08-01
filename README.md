# Yet Another Driver Station

A modern, cross-platform FRC Driver Station application built with Qt/QML, designed to provide teams with a reliable and feature-rich interface for controlling their robots during competitions and practice sessions.

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
- Qt 6.5.0 or later with QML support
- CMake 3.16 or later
- C++17 compatible compiler
- Git

#### Platform-specific Dependencies

**Windows:**
- Visual Studio 2019 or later
- Windows SDK

**macOS:**
- Xcode Command Line Tools
- macOS SDK

**Linux:**
- Build essentials: `sudo apt-get install build-essential cmake`
- Development libraries: `sudo apt-get install libudev-dev libgl1-mesa-dev libxkbcommon-dev libx11-dev libxtst-dev`

#### Build Steps

1. **Clone the repository:**
   ```bash
   git clone https://github.com/thenetworkgrinch/yet-another-driver-station.git
   cd yet-another-driver-station
   ```

2. **Build QHotkey dependency:**
   
   **Linux/macOS:**
   ```bash
   chmod +x scripts/build-qhotkey.sh
   ./scripts/build-qhotkey.sh Release
   ```
   
   **Windows (PowerShell):**
   ```powershell
   .\scripts\build-qhotkey.ps1 -BuildType Release
   ```

3. **Build the application:**
   ```bash
   qmake YetAnotherDriverStation.pro CONFIG+=release
   make  # or 'nmake' on Windows
   ```

4. **Run the application:**
   ```bash
   ./build/release/YetAnotherDriverStation  # Linux/macOS
   .\build\release\YetAnotherDriverStation.exe  # Windows
   ```

#### Build Configuration Options

You can disable specific features during build:

```bash
# Disable all optional features
qmake YetAnotherDriverStation.pro CONFIG+=release \
    CONFIG+=no_fms_support \
    CONFIG+=no_glass_integration \
    CONFIG+=no_dashboard_management \
    CONFIG+=no_practice_match \
    CONFIG+=no_global_shortcuts

# Disable only global shortcuts
qmake YetAnotherDriverStation.pro CONFIG+=release CONFIG+=no_global_shortcuts
```

## Usage

### First Time Setup
1. Launch the application
2. Enter your team number in the settings
3. Connect your robot via Ethernet
4. Configure any controllers in the Controllers tab

### Basic Operation
1. **Enable Robot**: Click the "Enable" button or press Space
2. **Disable Robot**: Click "Disable" or press Space again
3. **Emergency Stop**: Press Enter or Ctrl+E for immediate stop
4. **Mode Selection**: Choose Autonomous, Teleop, or Test mode
5. **Monitor Status**: Watch the status indicators for robot and network health

### Global Shortcuts
These shortcuts work system-wide, even when the application isn't focused:
- **Space**: Toggle robot enable/disable
- **Enter**: Emergency stop (disable robot)
- **Ctrl+E**: Emergency stop (alternative)

### Practice Matches
1. Go to the Operations tab
2. Click "Start Practice Match"
3. The timer will automatically cycle through Autonomous and Teleop periods
4. Use this to simulate real match conditions

## Configuration

### Team Settings
- Set your team number in Settings → Team Configuration
- The application will automatically calculate robot IP addresses

### Network Configuration
- Default robot IP: `10.TE.AM.2` (where TEAM is your team number)
- FMS IP: Automatically detected during competitions
- Dashboard ports: Configurable per dashboard type

### Controller Configuration
- Controllers are automatically detected when plugged in
- Mapping can be customized in the Controllers tab
- Supports Xbox, PlayStation, and most HID-compliant controllers

### Dashboard Integration
The application supports multiple dashboard types:
- **SmartDashboard**: Classic LabVIEW-based dashboard
- **Shuffleboard**: Modern Java-based dashboard
- **Glass**: Web-based dashboard with advanced features
- **Custom**: Support for team-specific dashboards

## Troubleshooting

### Common Issues

**Robot not connecting:**
- Verify team number is correct
- Check Ethernet cable connection
- Ensure robot is powered on and running code
- Check firewall settings

**Controllers not detected:**
- Try unplugging and reconnecting the controller
- Check if the controller works in other applications
- Verify USB cable is functional

**Global shortcuts not working:**
- Check if another application is using the same shortcuts
- On Linux, ensure X11 is running (Wayland support is limited)
- Try running the application as administrator (Windows only)

**Build failures:**
- Ensure all dependencies are installed
- Check Qt version compatibility
- Verify CMake and compiler versions
- Try cleaning the build directory

### Getting Help
- Check the [Issues](https://github.com/thenetworkgrinch/yet-another-driver-station/issues) page
- Join our [Discord community](https://discord.gg/your-invite)
- Email support: support@your-domain.com

## Development

### Project Structure
```
yet-another-driver-station/
├── backend/                 # C++ backend code
│   ├── core/               # Core utilities and logging
│   ├── comms/              # Robot communication
│   ├── controllers/        # Controller handling
│   ├── managers/           # System managers
│   └── fms/                # FMS integration
├── qml/                    # QML user interface
├── dashboards/             # Dashboard configurations
├── scripts/                # Build and utility scripts
├── thirdparty/            # Third-party dependencies
└── .github/               # CI/CD workflows
```

### Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

### Code Style
- C++: Follow Qt coding conventions
- QML: Use Qt Quick best practices
- Comments: Document public APIs and complex logic
- Testing: Add unit tests for new functionality

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **FIRST Robotics Competition** for the protocol specifications
- **Qt Project** for the excellent cross-platform framework
- **QHotkey** library for global shortcut support
- **FRC Community** for feedback and contributions

## Roadmap

### Version 2025.2.0
- [ ] Advanced telemetry visualization
- [ ] Plugin system for custom extensions
- [ ] Enhanced logging and replay capabilities
- [ ] Mobile companion app

### Version 2025.3.0
- [ ] Machine learning-based network optimization
- [ ] Advanced robot diagnostics
- [ ] Team collaboration features
- [ ] Cloud-based configuration sync

---

**Built with ❤️ for the FRC Community**
