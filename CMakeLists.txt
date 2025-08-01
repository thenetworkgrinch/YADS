cmake_minimum_required(VERSION 3.16)

project(YetAnotherDriverStation VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required Qt components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Qml Quick QuickControls2 Network Multimedia)

# Feature options
option(ENABLE_GLOBAL_SHORTCUTS "Enable global shortcuts support" ON)
option(ENABLE_FMS_SUPPORT "Enable FMS communication support" ON)
option(ENABLE_GLASS_INTEGRATION "Enable Glass dashboard integration" ON)
option(ENABLE_DASHBOARD_MANAGEMENT "Enable dashboard management" ON)
option(ENABLE_PRACTICE_MATCH "Enable practice match functionality" ON)

# Add QHotkey as subdirectory if global shortcuts are enabled
if(ENABLE_GLOBAL_SHORTCUTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/QHotkey/CMakeLists.txt")
        add_subdirectory(thirdparty/QHotkey)
        set(QHOTKEY_AVAILABLE TRUE)
    else()
        message(WARNING "QHotkey not found. Global shortcuts will be disabled.")
        set(ENABLE_GLOBAL_SHORTCUTS OFF)
        set(QHOTKEY_AVAILABLE FALSE)
    endif()
endif()

# Define the executable
qt6_add_executable(YetAnotherDriverStation
    main.cpp
    backend/core/logger.cpp
    backend/robotstate.cpp
    backend/fms/fmshandler.cpp
    backend/robot/comms/fms/fmshandler.cpp
    backend/robot/comms/packets.cpp
    backend/robot/comms/communicationhandler.cpp
    backend/controllers/controllerhidhandler.cpp
    backend/controllers/controllerhiddevice.cpp
    backend/managers/battery_manager.cpp
    backend/managers/practice_match_manager.cpp
    backend/managers/network_manager.cpp
    backend/comms/communicationhandler.cpp
    backend/comms/packets.cpp
)

# Add QML resources
qt6_add_qml_module(YetAnotherDriverStation
    URI YetAnotherDriverStation
    VERSION 1.0
    QML_FILES
        main.qml
        qml/OperationsView.qml
        qml/DiagnosticsView.qml
        qml/ControllersView.qml
        qml/ChartsView.qml
        qml/JoysticksView.qml
        qml/DashboardView.qml
        qml/NetworkDiagnosticsView.qml
        qml/LoggingView.qml
        qml/SystemStatusIndicator.qml
        qml/StatusIndicator.qml
        qml/NetworkTablesIndicator.qml
        qml/LogDownloadDialog.qml
        qml/SystemDetailsDialog.qml
        qml/BatteryAlertDialog.qml
        qml/BatterySettingsDialog.qml
)

# Link Qt libraries
target_link_libraries(YetAnotherDriverStation PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Multimedia
)

# Link QHotkey if available
if(QHOTKEY_AVAILABLE)
    target_link_libraries(YetAnotherDriverStation PRIVATE QHotkey)
    target_compile_definitions(YetAnotherDriverStation PRIVATE QHOTKEY_AVAILABLE)
endif()

# Feature definitions
if(ENABLE_GLOBAL_SHORTCUTS)
    target_compile_definitions(YetAnotherDriverStation PRIVATE ENABLE_GLOBAL_SHORTCUTS)
endif()

if(ENABLE_FMS_SUPPORT)
    target_compile_definitions(YetAnotherDriverStation PRIVATE ENABLE_FMS_SUPPORT)
endif()

if(ENABLE_GLASS_INTEGRATION)
    target_compile_definitions(YetAnotherDriverStation PRIVATE ENABLE_GLASS_INTEGRATION)
endif()

if(ENABLE_DASHBOARD_MANAGEMENT)
    target_compile_definitions(YetAnotherDriverStation PRIVATE ENABLE_DASHBOARD_MANAGEMENT)
endif()

if(ENABLE_PRACTICE_MATCH)
    target_compile_definitions(YetAnotherDriverStation PRIVATE ENABLE_PRACTICE_MATCH)
endif()

# Include directories
target_include_directories(YetAnotherDriverStation PRIVATE
    backend
    backend/core
    backend/comms
    backend/robot
    backend/robot/comms
    backend/robot/comms/fms
    backend/fms
    backend/controllers
    backend/managers
)

# Platform-specific configurations
if(WIN32)
    target_link_libraries(YetAnotherDriverStation PRIVATE ws2_32 wsock32 hid setupapi)
    target_compile_definitions(YetAnotherDriverStation PRIVATE WIN32_LEAN_AND_MEAN)
endif()

if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(UDEV REQUIRED libudev)
    target_link_libraries(YetAnotherDriverStation PRIVATE ${UDEV_LIBRARIES})
    target_include_directories(YetAnotherDriverStation PRIVATE ${UDEV_INCLUDE_DIRS})
endif()

if(APPLE)
    target_link_libraries(YetAnotherDriverStation PRIVATE
        "-framework IOKit"
        "-framework CoreFoundation"
    )
endif()

# Set target properties
set_target_properties(YetAnotherDriverStation PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Install rules
install(TARGETS YetAnotherDriverStation
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
