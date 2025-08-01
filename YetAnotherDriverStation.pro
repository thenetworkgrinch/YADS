QT += core widgets qml quick network

CONFIG += c++17

TARGET = YetAnotherDriverStation
TEMPLATE = app

# Version information
VERSION = 2025.1.0
QMAKE_TARGET_COMPANY = "FRC Community"
QMAKE_TARGET_PRODUCT = "Yet Another Driver Station"
QMAKE_TARGET_DESCRIPTION = "Modern FRC Driver Station Application"
QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2025 FRC Community"

# Feature flags - can be disabled by adding CONFIG+=no_fms_support etc.
!CONFIG(no_fms_support): DEFINES += ENABLE_FMS_SUPPORT=1
!CONFIG(no_glass_integration): DEFINES += ENABLE_GLASS_INTEGRATION=1
!CONFIG(no_dashboard_management): DEFINES += ENABLE_DASHBOARD_MANAGEMENT=1
!CONFIG(no_practice_match): DEFINES += ENABLE_PRACTICE_MATCH=1
!CONFIG(no_global_shortcuts): DEFINES += ENABLE_GLOBAL_SHORTCUTS=1

# QHotkey dependency via qpmx
!CONFIG(no_global_shortcuts) {
    include($$PWD/vendor/qpmx/qpmx.pri)
    QPMX_DEPENDENCIES += de.skycoder42.qhotkey
}

# Source files
SOURCES += \
    main.cpp \
    backend/robotstate.cpp \
    backend/core/logger.cpp \
    backend/comms/packets.cpp \
    backend/comms/communicationhandler.cpp \
    backend/controllers/controllerhidhandler.cpp \
    backend/controllers/controllerhiddevice.cpp \
    backend/managers/battery_manager.cpp \
    backend/managers/practice_match_manager.cpp \
    backend/managers/network_manager.cpp

# Header files
HEADERS += \
    backend/robotstate.h \
    backend/core/logger.h \
    backend/core/constants.h \
    backend/comms/packets.h \
    backend/comms/communicationhandler.h \
    backend/controllers/controllerhidhandler.h \
    backend/controllers/controllerhiddevice.h \
    backend/managers/battery_manager.h \
    backend/managers/practice_match_manager.h \
    backend/managers/network_manager.h

# FMS support (conditional compilation)
contains(DEFINES, ENABLE_FMS_SUPPORT=1) {
    SOURCES += backend/fms/fmshandler.cpp
    HEADERS += backend/fms/fmshandler.h
}

# Platform-specific libraries
win32 {
    LIBS += -lhid -lsetupapi -lws2_32
    DEFINES += WIN32_LEAN_AND_MEAN
}

unix:!macx {
    LIBS += -ludev
    PKGCONFIG += libudev
}

macx {
    LIBS += -framework IOKit -framework CoreFoundation
    # ARM64 only support
    QMAKE_APPLE_DEVICE_ARCHS = arm64
}

# Resources
RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Build configuration
CONFIG(debug, debug|release) {
    DEFINES += DEBUG_BUILD
    TARGET = $${TARGET}_debug
}

CONFIG(release, debug|release) {
    DEFINES += RELEASE_BUILD
    QMAKE_CXXFLAGS_RELEASE += -O2
}

# Compiler warnings
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic

# Include paths
INCLUDEPATH += backend

# Output directories
CONFIG(debug, debug|release) {
    DESTDIR = build/debug
    OBJECTS_DIR = build/debug/obj
    MOC_DIR = build/debug/moc
    RCC_DIR = build/debug/rcc
    UI_DIR = build/debug/ui
}

CONFIG(release, debug|release) {
    DESTDIR = build/release
    OBJECTS_DIR = build/release/obj
    MOC_DIR = build/release/moc
    RCC_DIR = build/release/rcc
    UI_DIR = build/release/ui
}
