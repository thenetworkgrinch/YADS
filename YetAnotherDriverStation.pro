QT += core gui widgets network qml quick quickcontrols2 multimedia
CONFIG += c++17

TARGET = YetAnotherDriverStation
TEMPLATE = app

# Version information
VERSION = 1.0.0
QMAKE_TARGET_COMPANY = "FRC Team"
QMAKE_TARGET_PRODUCT = "Yet Another Driver Station"
QMAKE_TARGET_DESCRIPTION = "Advanced FRC Driver Station Application"
QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2024"

# Build configuration
CONFIG(debug, debug|release) {
    DESTDIR = build/debug
    OBJECTS_DIR = build/debug/obj
    MOC_DIR = build/debug/moc
    RCC_DIR = build/debug/rcc
    UI_DIR = build/debug/ui
} else {
    DESTDIR = build/release
    OBJECTS_DIR = build/release/obj
    MOC_DIR = build/release/moc
    RCC_DIR = build/release/rcc
    UI_DIR = build/release/ui
}

# QHotkey integration
INCLUDEPATH += thirdparty/QHotkey/src
LIBS += -L$$PWD/thirdparty/QHotkey/build -lQHotkey

# Platform-specific configurations
win32 {
    LIBS += -luser32 -ladvapi32
    RC_ICONS = resources/icons/app.ico
}

unix:!macx {
    LIBS += -lX11 -lXtst
    TARGET.path = /usr/local/bin
    INSTALLS += TARGET
}

macx {
    LIBS += -framework Carbon -framework ApplicationServices
    ICON = resources/icons/app.icns
}

# Feature flags
!contains(CONFIG, no_global_shortcuts) {
    DEFINES += ENABLE_GLOBAL_SHORTCUTS
}

!contains(CONFIG, no_fms_support) {
    DEFINES += ENABLE_FMS_SUPPORT
}

!contains(CONFIG, no_glass_integration) {
    DEFINES += ENABLE_GLASS_INTEGRATION  
}

!contains(CONFIG, no_dashboard_management) {
    DEFINES += ENABLE_DASHBOARD_MANAGEMENT
}

!contains(CONFIG, no_practice_match) {
    DEFINES += ENABLE_PRACTICE_MATCH
}

# Source files
SOURCES += \
    main.cpp \
    backend/core/logger.cpp \
    backend/robotstate.cpp \
    backend/fms/fmshandler.cpp \
    backend/robot/comms/fms/fmshandler.cpp \
    backend/robot/comms/packets.cpp \
    backend/robot/comms/communicationhandler.cpp \
    backend/controllers/controllerhidhandler.cpp \
    backend/controllers/controllerhiddevice.cpp \
    backend/managers/battery_manager.cpp \
    backend/managers/practice_match_manager.cpp \
    backend/managers/network_manager.cpp \
    backend/comms/communicationhandler.cpp \
    backend/comms/packets.cpp

# Header files
HEADERS += \
    backend/core/constants.h \
    backend/core/logger.h \
    backend/robotstate.h \
    backend/fms/fmshandler.h \
    backend/robot/comms/fms/fmshandler.h \
    backend/robot/comms/packets.h \
    backend/robot/comms/communicationhandler.h \
    backend/controllers/controllerhidhandler.h \
    backend/controllers/controllerhiddevice.h \
    backend/managers/battery_manager.h \
    backend/managers/practice_match_manager.h \
    backend/managers/network_manager.h \
    backend/comms/communicationhandler.h \
    backend/comms/packets.h

# Resources
RESOURCES += qml.qrc

# Additional files
DISTFILES += \
    qml/*.qml \
    dashboards/windows/dashboards.json \
    dashboards/macos/dashboards.json \
    dashboards/linux/dashboards.json \
    README.md
