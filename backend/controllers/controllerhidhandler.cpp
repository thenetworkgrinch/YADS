#include "controllerhidhandler.h"
#include "../core/logger.h"
#include "../core/constants.h"
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <hidsdi.h>
#include <setupapi.h>
#elif defined(Q_OS_LINUX)
#include <libudev.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#elif defined(Q_OS_MACOS)
#include <IOKit/hid/IOHIDManager.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace FRCDriverStation;
using namespace FRCDriverStation::Constants;

ControllerHIDHandler::ControllerHIDHandler(std::shared_ptr<Logger> logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_pollTimer(std::make_unique<QTimer>(this))
    , m_detectTimer(std::make_unique<QTimer>(this))
    , m_polling(false)
{
    // Setup polling timer
    m_pollTimer->setInterval(Controllers::CONTROLLER_POLL_INTERVAL_MS);
    connect(m_pollTimer.get(), &QTimer::timeout, this, &ControllerHIDHandler::pollControllers);
    
    // Setup detection timer (check for new controllers every 2 seconds)
    m_detectTimer->setInterval(2000);
    connect(m_detectTimer.get(), &QTimer::timeout, this, &ControllerHIDHandler::detectControllers);
    
    m_logger->info("Controller Handler", "Controller HID handler initialized");
}

ControllerHIDHandler::~ControllerHIDHandler()
{
    stopPolling();
    m_logger->info("Controller Handler", "Controller HID handler destroyed");
}

void ControllerHIDHandler::startPolling()
{
    if (m_polling) return;
    
    m_polling = true;
    refreshControllers();
    m_pollTimer->start();
    m_detectTimer->start();
    
    m_logger->info("Controller Handler", "Started controller polling");
}

void ControllerHIDHandler::stopPolling()
{
    if (!m_polling) return;
    
    m_polling = false;
    m_pollTimer->stop();
    m_detectTimer->stop();
    
    m_logger->info("Controller Handler", "Stopped controller polling");
}

void ControllerHIDHandler::refreshControllers()
{
    // Get current list of HID devices
    QList<ControllerHIDDevice*> currentDevices = enumerateHIDDevices();
    
    // Check for new controllers
    for (ControllerHIDDevice *device : currentDevices) {
        if (!m_controllers.contains(device->deviceId())) {
            addController(device);
        }
    }
    
    // Check for disconnected controllers
    QStringList disconnectedIds;
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        bool found = false;
        for (ControllerHIDDevice *device : currentDevices) {
            if (device->deviceId() == it.key()) {
                found = true;
                break;
            }
        }
        if (!found) {
            disconnectedIds.append(it.key());
        }
    }
    
    // Remove disconnected controllers
    for (const QString &deviceId : disconnectedIds) {
        removeController(deviceId);
    }
    
    // Clean up temporary devices that weren't added
    for (ControllerHIDDevice *device : currentDevices) {
        if (!m_controllers.contains(device->deviceId())) {
            delete device;
        }
    }
}

bool ControllerHIDHandler::bindControllerToSlot(const QString &deviceId, int slot)
{
    if (slot < 0 || slot >= MAX_CONTROLLER_SLOTS) {
        m_logger->warning("Controller Binding", "Invalid slot number", QString("Slot: %1").arg(slot));
        return false;
    }
    
    ControllerHIDDevice *controller = getControllerById(deviceId);
    if (!controller) {
        m_logger->warning("Controller Binding", "Controller not found", deviceId);
        return false;
    }
    
    // Unbind any existing controller from this slot
    if (m_slotBindings.contains(slot)) {
        unbindControllerFromSlot(slot);
    }
    
    // Unbind this controller from any existing slot
    int currentSlot = getSlotForController(deviceId);
    if (currentSlot >= 0) {
        unbindControllerFromSlot(currentSlot);
    }
    
    // Bind controller to slot
    m_slotBindings[slot] = controller;
    
    m_logger->info("Controller Binding", "Controller bound to slot", 
                  QString("Device: %1, Slot: %2").arg(controller->name()).arg(slot));
    
    emit controllerBound(controller, slot);
    return true;
}

void ControllerHIDHandler::unbindControllerFromSlot(int slot)
{
    if (!m_slotBindings.contains(slot)) {
        return;
    }
    
    ControllerHIDDevice *controller = m_slotBindings[slot];
    QString deviceId = controller->deviceId();
    
    m_slotBindings.remove(slot);
    
    m_logger->info("Controller Binding", "Controller unbound from slot", 
                  QString("Device: %1, Slot: %2").arg(controller->name()).arg(slot));
    
    emit controllerUnbound(deviceId, slot);
}

ControllerHIDDevice* ControllerHIDHandler::getControllerInSlot(int slot) const
{
    return m_slotBindings.value(slot, nullptr);
}

int ControllerHIDHandler::getSlotForController(const QString &deviceId) const
{
    for (auto it = m_slotBindings.begin(); it != m_slotBindings.end(); ++it) {
        if (it.value()->deviceId() == deviceId) {
            return it.key();
        }
    }
    return -1;
}

QList<ControllerHIDDevice*> ControllerHIDHandler::getAllControllers() const
{
    QList<ControllerHIDDevice*> controllers;
    for (const auto &controller : m_controllers) {
        controllers.append(controller.get());
    }
    return controllers;
}

QList<ControllerHIDDevice*> ControllerHIDHandler::getAllBoundControllers() const
{
    return m_slotBindings.values();
}

QList<ControllerHIDDevice*> ControllerHIDHandler::getUnboundControllers() const
{
    QList<ControllerHIDDevice*> unbound;
    for (const auto &controller : m_controllers) {
        if (getSlotForController(controller->deviceId()) < 0) {
            unbound.append(controller.get());
        }
    }
    return unbound;
}

ControllerHIDDevice* ControllerHIDHandler::getControllerById(const QString &deviceId) const
{
    auto it = m_controllers.find(deviceId);
    return (it != m_controllers.end()) ? it.value().get() : nullptr;
}

int ControllerHIDHandler::getBoundControllerCount() const
{
    return m_slotBindings.size();
}

void ControllerHIDHandler::pollControllers()
{
    updateControllerData();
}

void ControllerHIDHandler::detectControllers()
{
    refreshControllers();
}

void ControllerHIDHandler::addController(ControllerHIDDevice *controller)
{
    QString deviceId = controller->deviceId();
    m_controllers[deviceId] = std::unique_ptr<ControllerHIDDevice>(controller);
    
    m_logger->info("Controller Detection", "Controller connected", 
                  QString("Name: %1, ID: %2").arg(controller->name()).arg(deviceId));
    
    emit controllerConnected(controller);
}

void ControllerHIDHandler::removeController(const QString &deviceId)
{
    auto it = m_controllers.find(deviceId);
    if (it == m_controllers.end()) {
        return;
    }
    
    ControllerHIDDevice *controller = it.value().get();
    QString name = controller->name();
    
    // Unbind from slot if bound
    int slot = getSlotForController(deviceId);
    if (slot >= 0) {
        unbindControllerFromSlot(slot);
    }
    
    // Remove from controllers map
    m_controllers.erase(it);
    
    m_logger->info("Controller Detection", "Controller disconnected", 
                  QString("Name: %1, ID: %2").arg(name).arg(deviceId));
    
    emit controllerDisconnected(deviceId);
}

void ControllerHIDHandler::updateControllerData()
{
    // Update data for all bound controllers
    for (auto it = m_slotBindings.begin(); it != m_slotBindings.end(); ++it) {
        int slot = it.key();
        ControllerHIDDevice *controller = it.value();
        
        if (controller && controller->isConnected()) {
            if (controller->updateData()) {
                emit controllerDataChanged(slot);
            }
        }
    }
}

QList<ControllerHIDDevice*> ControllerHIDHandler::enumerateHIDDevices()
{
    QList<ControllerHIDDevice*> devices;
    
#ifdef Q_OS_WIN
    // Windows HID enumeration
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);
    
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, 
                                                 DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return devices;
    }
    
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &hidGuid, i, &deviceInterfaceData); i++) {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);
        
        if (requiredSize > 0) {
            PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = 
                (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
            deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            
            if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, 
                                              deviceInterfaceDetailData, requiredSize, NULL, NULL)) {
                
                QString devicePath = QString::fromWCharArray(deviceInterfaceDetailData->DevicePath);
                ControllerHIDDevice *device = ControllerHIDDevice::createFromPath(devicePath, m_logger);
                
                if (device && device->isGameController()) {
                    devices.append(device);
                } else {
                    delete device;
                }
            }
            
            free(deviceInterfaceDetailData);
        }
    }
    
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    
#elif defined(Q_OS_LINUX)
    // Linux udev enumeration
    struct udev *udev = udev_new();
    if (!udev) {
        return devices;
    }
    
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);
    
    struct udev_list_entry *dev_list_entry;
    udev_list_entry_foreach(dev_list_entry, udev_enumerate_get_list_entry(enumerate)) {
        const char *path = udev_list_entry_get_name(dev_list_entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);
        
        const char *devnode = udev_device_get_devnode(dev);
        if (devnode && strstr(devnode, "/dev/input/event")) {
            QString devicePath = QString::fromUtf8(devnode);
            ControllerHIDDevice *device = ControllerHIDDevice::createFromPath(devicePath, m_logger);
            
            if (device && device->isGameController()) {
                devices.append(device);
            } else {
                delete device;
            }
        }
        
        udev_device_unref(dev);
    }
    
    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    
#elif defined(Q_OS_MACOS)
    // macOS IOKit HID enumeration
    IOHIDManagerRef hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    
    if (hidManager) {
        IOHIDManagerSetDeviceMatching(hidManager, NULL);
        IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
        
        CFSetRef deviceSet = IOHIDManagerCopyDevices(hidManager);
        if (deviceSet) {
            CFIndex deviceCount = CFSetGetCount(deviceSet);
            IOHIDDeviceRef *deviceArray = (IOHIDDeviceRef*)malloc(sizeof(IOHIDDeviceRef) * deviceCount);
            CFSetGetValues(deviceSet, (const void**)deviceArray);
            
            for (CFIndex i = 0; i < deviceCount; i++) {
                IOHIDDeviceRef device = deviceArray[i];
                ControllerHIDDevice *controller = ControllerHIDDevice::createFromIOHIDDevice(device, m_logger);
                
                if (controller && controller->isGameController()) {
                    devices.append(controller);
                } else {
                    delete controller;
                }
            }
            
            free(deviceArray);
            CFRelease(deviceSet);
        }
        
        IOHIDManagerClose(hidManager, kIOHIDOptionsTypeNone);
        CFRelease(hidManager);
    }
#endif
    
    return devices;
}
