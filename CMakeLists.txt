cmake_minimum_required(VERSION 3.21)

# Set Qt policy to suppress QML resource prefix warning
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()

project(YetAnotherDriverStation VERSION 2025.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Set Qt policy for QML modules
qt_policy(SET QTP0001 NEW)

# Find Qt6 components
find_package(Qt6 6.5.0 REQUIRED COMPONENTS
    Core
    Quick
    QuickControls2
    Network
    Multimedia
    Charts
    Widgets
)

# Feature options
option(ENABLE_GLOBAL_SHORTCUTS "Enable global shortcut support" ON)
option(ENABLE_FMS_SUPPORT "Enable FMS integration" ON)
option(ENABLE_GLASS_INTEGRATION "Enable Glass dashboard support" ON)
option(ENABLE_DASHBOARD_MANAGEMENT "Enable dashboard management" ON)
option(ENABLE_PRACTICE_MATCH "Enable practice match functionality" ON)
option(ENABLE_DEBUG_LOGGING "Enable debug logging" OFF)
option(ENABLE_UNIT_TESTS "Build unit tests" OFF)

# Clone QHotkey if global shortcuts are enabled
if(ENABLE_GLOBAL_SHORTCUTS)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/QHotkey/CMakeLists.txt")
        find_package(Git REQUIRED)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} clone --depth 1 https://github.com/Skycoder42/QHotkey.git thirdparty/QHotkey
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE GIT_RESULT
        )
        if(NOT GIT_RESULT EQUAL "0")
            message(FATAL_ERROR "Failed to clone QHotkey dependency")
        endif()
    endif()
    add_subdirectory(thirdparty/QHotkey)
endif()

# Define source files
set(SOURCES
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
    backend/managers/application_manager.cpp
    backend/robot/comms/communicationhandler.cpp
    backend/robot/comms/packets.cpp
)

# Define header files
set(HEADERS
    backend/core/constants.h
    backend/core/logger.h
    backend/robotstate.h
    backend/fms/fmshandler.h
    backend/robot/comms/fms/fmshandler.h
    backend/robot/comms/packets.h
    backend/robot/comms/communicationhandler.h
    backend/controllers/controllerhidhandler.h
    backend/controllers/controllerhiddevice.h
    backend/managers/battery_manager.h
    backend/managers/practice_match_manager.h
    backend/managers/network_manager.h
    backend/managers/application_manager.h
)

# Create the executable first
qt6_add_executable(YetAnotherDriverStation ${SOURCES} ${HEADERS})

# Add QML module
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
    RESOURCES
        qml.qrc
)

# Link Qt libraries
target_link_libraries(YetAnotherDriverStation PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Multimedia
    Qt6::Charts
    Qt6::Widgets
)

# Link QHotkey if enabled
if(ENABLE_GLOBAL_SHORTCUTS)
    target_link_libraries(YetAnotherDriverStation PRIVATE QHotkey)
    target_compile_definitions(YetAnotherDriverStation PRIVATE ENABLE_GLOBAL_SHORTCUTS)
endif()

# Feature definitions
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

if(ENABLE_DEBUG_LOGGING)
    target_compile_definitions(YetAnotherDriverStation PRIVATE ENABLE_DEBUG_LOGGING)
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
    target_compile_definitions(YetAnotherDriverStation PRIVATE WIN32_LEAN_AND_MEAN)
    set_target_properties(YetAnotherDriverStation PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(APPLE)
    set_target_properties(YetAnotherDriverStation PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.in
    )
endif()

# Install configuration
install(TARGETS YetAnotherDriverStation
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# Unit tests
if(ENABLE_UNIT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
