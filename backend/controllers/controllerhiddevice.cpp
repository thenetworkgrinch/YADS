#include "controllerhiddevice.h"
#include "../core/logger.h"
#include "../core/constants.h"
#include <QDebug>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <hidsdi.h>
#include <setupapi.h>
#elif defined(Q_OS_LINUX)
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#elif defined(Q_OS_MACOS)
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace FRCDriverStation;
using namespace FRCDriverStation::Constants;

ControllerHIDDevice::ControllerHIDDevice(const QString &devicePath, std::shared_ptr<Logger> logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_devicePath(devicePath)
    , m_vendorId(0)
    , m_productId(0)
    , m_connected(false)
    , m_axisCount(0)
    , m_buttonCount(0)
    , m_povCount(0)
#ifdef Q_OS_WIN
    , m_deviceHandle(INVALID_HANDLE_VALUE)
    , m_preparsedData(nullptr)
#elif defined(Q_OS_LINUX)
    , m_deviceFd(-1)
#elif defined(Q_OS_MACOS)
    , m_hidDevice(nullptr)
#endif
{
    // Generate device ID from path
    m_deviceId = QString::number(qHash(devicePath));
    
    // Initialize data vectors
    m_axisValues.resize(Controllers::MAX_AXES_PER_CONTROLLER, 0.0f);
    m_buttonValues.resize(Controllers::MAX_BUTTONS_PER_CONTROLLER, false);
    m_povValues.resize(Controllers::MAX_POVS_PER_CONTROLLER, -1);
}

ControllerHIDDevice::~ControllerHIDDevice()
{
    closeDevice();
}

ControllerHIDDevice* ControllerHIDDevice::createFromPath(const QString &devicePath, std::shared_ptr<Logger> logger)
{
    ControllerHIDDevice *device = new ControllerHIDDevice(devicePath, logger);
    
    if (device->openDevice() && device->readDeviceInfo() && device->readCapabilities()) {
        device->m_connected = true;
        logger->debug("Controller Device", "Device created successfully", 
                     QString("Path: %1, Name: %2").arg(devicePath).arg(device->m_name));
        return device;
    } else {
        logger->debug("Controller Device", "Failed to create device", devicePath);
        delete device;
        return nullptr;
    }
}

#ifdef Q_OS_MACOS
ControllerHIDDevice* ControllerHIDDevice::createFromIOHIDDevice(IOHIDDeviceRef hidDevice, std::shared_ptr> logger)
{
    // Get device path from IOHIDDevice
    CFStringRef pathRef = (CFStringRef)IOHIDDeviceGetProperty(hidDevice, CFSTR(kIOHIDLocationIDKey));
    QString devicePath;
    if (pathRef) {
        CFIndex length = CFStringGetLength(pathRef);
        CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        char *buffer = new char[maxSize];
        if (CFStringGetCString(pathRef, buffer, maxSize, kCFStringEncodingUTF8)) {
            devicePath = QString::fromUtf8(buffer);
        }
        delete[] buffer;
    }
    
    if (devicePath.isEmpty()) {
        devicePath = QString("macos_hid_%1").arg(reinterpret_cast<quintptr>(hidDevice));
    }
    
    ControllerHIDDevice *device = new ControllerHIDDevice(devicePath, logger);
    device->m_hidDevice = hidDevice;
    CFRetain(hidDevice);
    
    if (device->readDeviceInfo() && device->readCapabilities()) {
        device->m_connected = true;
        logger->debug("Controller Device", "macOS device created successfully", 
                     QString("Name: %1").arg(device->m_name));
        return device;
    } else {
        logger->debug("Controller Device", "Failed to create macOS device");
        delete device;
        return nullptr;
    }
}
#endif

bool ControllerHIDDevice::isGameController() const
{
    // Check if device has typical game controller characteristics
    if (m_axisCount >= 2 && m_buttonCount >= 4) {
        return true;
    }
    
    // Check vendor/product ID for known game controllers
    const QList<QPair<quint16, quint16>> knownControllers = {
        {0x045E, 0x028E}, // Xbox 360 Controller
        {0x045E, 0x02D1}, // Xbox One Controller
        {0x045E, 0x02DD}, // Xbox One Controller (2016)
        {0x045E, 0x0719}, // Xbox 360 Wireless Controller
        {0x054C, 0x0268}, // PlayStation 3 Controller
        {0x054C, 0x05C4}, // PlayStation 4 Controller
        {0x054C, 0x09CC}, // PlayStation 4 Controller v2
        {0x057E, 0x0337}, // Nintendo GameCube Controller Adapter
        {0x057E, 0x2009}, // Nintendo Switch Pro Controller
        {0x046D, 0xC21D}, // Logitech F310
        {0x046D, 0xC21E}, // Logitech F510
        {0x046D, 0xC21F}, // Logitech F710
    };
    
    for (const auto &controller : knownControllers) {
        if (m_vendorId == controller.first && m_productId == controller.second) {
            return true;
        }
    }
    
    return false;
}

float ControllerHIDDevice::getAxisValue(int axis) const
{
    if (axis >= 0 && axis < m_axisValues.size()) {
        return m_axisValues[axis];
    }
    return 0.0f;
}

bool ControllerHIDDevice::getButtonValue(int button) const
{
    if (button >= 0 && button < m_buttonValues.size()) {
        return m_buttonValues[button];
    }
    return false;
}

qint16 ControllerHIDDevice::getPOVValue(int pov) const
{
    if (pov >= 0 && pov < m_povValues.size()) {
        return m_povValues[pov];
    }
    return -1;
}

bool ControllerHIDDevice::updateData()
{
    if (!m_connected) {
        return false;
    }
    
#ifdef Q_OS_WIN
    // Windows HID data reading
    UCHAR reportBuffer[256];
    DWORD bytesRead;
    
    if (!ReadFile(m_deviceHandle, reportBuffer, sizeof(reportBuffer), &bytesRead, NULL)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            m_connected = false;
            emit disconnected();
            return false;
        }
    }
    
    // Parse HID report data
    // This is a simplified implementation - real implementation would parse the HID report descriptor
    // and extract axis/button data based on the device's specific layout
    
    return true;
    
#elif defined(Q_OS_LINUX)
    // Linux input event reading
    struct input_event events[64];
    ssize_t bytesRead = read(m_deviceFd, events, sizeof(events));
    
    if (bytesRead < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            m_connected = false;
            emit disconnected();
            return false;
        }
        return false;
    }
    
    int numEvents = bytesRead / sizeof(struct input_event);
    bool dataChanged = false;
    
    for (int i = 0; i < numEvents; i++) {
        const struct input_event &event = events[i];
        
        if (event.type == EV_ABS) {
            // Axis event
            for (int axis = 0; axis < m_axisCount && axis < m_axisTypes.size(); axis++) {
                if (event.code == m_axisTypes[axis]) {
                    normalizeAxisValue(axis, event.value);
                    dataChanged = true;
                    break;
                }
            }
        } else if (event.type == EV_KEY) {
            // Button event
            for (int button = 0; button < m_buttonCount && button < m_buttonTypes.size(); button++) {
                if (event.code == m_buttonTypes[button]) {
                    m_buttonValues[button] = (event.value != 0);
                    dataChanged = true;
                    break;
                }
            }
        }
    }
    
    return dataChanged;
    
#elif defined(Q_OS_MACOS)
    // macOS IOKit HID data reading
    bool dataChanged = false;
    
    // Read axis values
    for (int i = 0; i < m_axisElements.size() && i < m_axisCount; i++) {
        IOHIDValueRef valueRef;
        if (IOHIDDeviceGetValue(m_hidDevice, m_axisElements[i], &valueRef) == kIOReturnSuccess) {
            CFIndex rawValue = IOHIDValueGetIntegerValue(valueRef);
            
            // Get element properties for normalization
            CFIndex min = IOHIDElementGetLogicalMin(m_axisElements[i]);
            CFIndex max = IOHIDElementGetLogicalMax(m_axisElements[i]);
            
            // Normalize to -1.0 to 1.0 range
            float normalizedValue = 0.0f;
            if (max > min) {
                normalizedValue = (2.0f * (rawValue - min) / (max - min)) - 1.0f;
            }
            
            if (m_axisValues[i] != normalizedValue) {
                m_axisValues[i] = normalizedValue;
                dataChanged = true;
            }
        }
    }
    
    // Read button values
    for (int i = 0; i < m_buttonElements.size() && i < m_buttonCount; i++) {
        IOHIDValueRef valueRef;
        if (IOHIDDeviceGetValue(m_hidDevice, m_buttonElements[i], &valueRef) == kIOReturnSuccess) {
            bool pressed = IOHIDValueGetIntegerValue(valueRef) != 0;
            if (m_buttonValues[i] != pressed) {
                m_buttonValues[i] = pressed;
                dataChanged = true;
            }
        }
    }
    
    // Read POV values
    for (int i = 0; i < m_povElements.size() && i < m_povCount; i++) {
        IOHIDValueRef valueRef;
        if (IOHIDDeviceGetValue(m_hidDevice, m_povElements[i], &valueRef) == kIOReturnSuccess) {
            CFIndex rawValue = IOHIDValueGetIntegerValue(valueRef);
            
            // Convert to angle (0-359 degrees, -1 for not pressed)
            qint16 angle = -1;
            if (rawValue >= 0) {
                CFIndex min = IOHIDElementGetLogicalMin(m_povElements[i]);
                CFIndex max = IOHIDElementGetLogicalMax(m_povElements[i]);
                if (max > min) {
                    angle = static_cast<qint16>((rawValue - min) * 360 / (max - min + 1));
                }
            }
            
            if (m_povValues[i] != angle) {
                m_povValues[i] = angle;
                dataChanged = true;
            }
        }
    }
    
    return dataChanged;
#endif
    
    return false;
}

bool ControllerHIDDevice::openDevice()
{
#ifdef Q_OS_WIN
    m_deviceHandle = CreateFile(
        reinterpret_cast<LPCWSTR>(m_devicePath.utf16()),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );
    
    return m_deviceHandle != INVALID_HANDLE_VALUE;
    
#elif defined(Q_OS_LINUX)
    m_deviceFd = open(m_devicePath.toUtf8().constData(), O_RDONLY | O_NONBLOCK);
    return m_deviceFd >= 0;
    
#elif defined(Q_OS_MACOS)
    // Device is already opened by IOHIDManager
    return m_hidDevice != nullptr;
#endif
    
    return false;
}

void ControllerHIDDevice::closeDevice()
{
#ifdef Q_OS_WIN
    if (m_deviceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_deviceHandle);
        m_deviceHandle = INVALID_HANDLE_VALUE;
    }
    
    if (m_preparsedData) {
        HidD_FreePreparsedData(m_preparsedData);
        m_preparsedData = nullptr;
    }
    
#elif defined(Q_OS_LINUX)
    if (m_deviceFd >= 0) {
        close(m_deviceFd);
        m_deviceFd = -1;
    }
    
#elif defined(Q_OS_MACOS)
    if (m_hidDevice) {
        CFRelease(m_hidDevice);
        m_hidDevice = nullptr;
    }
    
    // Clean up element arrays
    for (IOHIDElementRef element : m_axisElements) {
        CFRelease(element);
    }
    for (IOHIDElementRef element : m_buttonElements) {
        CFRelease(element);
    }
    for (IOHIDElementRef element : m_povElements) {
        CFRelease(element);
    }
    
    m_axisElements.clear();
    m_buttonElements.clear();
    m_povElements.clear();
#endif
    
    m_connected = false;
}

bool ControllerHIDDevice::readDeviceInfo()
{
#ifdef Q_OS_WIN
    HIDD_ATTRIBUTES attributes;
    attributes.Size = sizeof(HIDD_ATTRIBUTES);
    
    if (!HidD_GetAttributes(m_deviceHandle, &attributes)) {
        return false;
    }
    
    m_vendorId = attributes.VendorID;
    m_productId = attributes.ProductID;
    
    // Get product string
    wchar_t productString[256];
    if (HidD_GetProductString(m_deviceHandle, productString, sizeof(productString))) {
        m_name = QString::fromWCharArray(productString);
    } else {
        m_name = QString("HID Device %1:%2").arg(m_vendorId, 4, 16, QChar('0')).arg(m_productId, 4, 16, QChar('0'));
    }
    
    return true;
    
#elif defined(Q_OS_LINUX)
    // Get device info using ioctl
    struct input_id id;
    if (ioctl(m_deviceFd, EVIOCGID, &id) < 0) {
        return false;
    }
    
    m_vendorId = id.vendor;
    m_productId = id.product;
    
    // Get device name
    char nameBuffer[256];
    if (ioctl(m_deviceFd, EVIOCGNAME(sizeof(nameBuffer)), nameBuffer) >= 0) {
        m_name = QString::fromUtf8(nameBuffer);
    } else {
        m_name = QString("Input Device %1:%2").arg(m_vendorId, 4, 16, QChar('0')).arg(m_productId, 4, 16, QChar('0'));
    }
    
    return true;
    
#elif defined(Q_OS_MACOS)
    // Get vendor ID
    CFNumberRef vendorIdRef = (CFNumberRef)IOHIDDeviceGetProperty(m_hidDevice, CFSTR(kIOHIDVendorIDKey));
    if (vendorIdRef) {
        CFNumberGetValue(vendorIdRef, kCFNumberSInt32Type, &m_vendorId);
    }
    
    // Get product ID
    CFNumberRef productIdRef = (CFNumberRef)IOHIDDeviceGetProperty(m_hidDevice, CFSTR(kIOHIDProductIDKey));
    if (productIdRef) {
        CFNumberGetValue(productIdRef, kCFNumberSInt32Type, &m_productId);
    }
    
    // Get product name
    CFStringRef productRef = (CFStringRef)IOHIDDeviceGetProperty(m_hidDevice, CFSTR(kIOHIDProductKey));
    if (productRef) {
        CFIndex length = CFStringGetLength(productRef);
        CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        char *buffer = new char[maxSize];
        if (CFStringGetCString(productRef, buffer, maxSize, kCFStringEncodingUTF8)) {
            m_name = QString::fromUtf8(buffer);
        }
        delete[] buffer;
    }
    
    if (m_name.isEmpty()) {
        m_name = QString("HID Device %1:%2").arg(m_vendorId, 4, 16, QChar('0')).arg(m_productId, 4, 16, QChar('0'));
    }
    
    return true;
#endif
    
    return false;
}

bool ControllerHIDDevice::readCapabilities()
{
#ifdef Q_OS_WIN
    // Get preparsed data
    if (!HidD_GetPreparsedData(m_deviceHandle, &m_preparsedData)) {
        return false;
    }
    
    HIDP_CAPS caps;
    if (HidP_GetCaps(m_preparsedData, &caps) != HIDP_STATUS_SUCCESS) {
        return false;
    }
    
    // This is a simplified implementation
    // Real implementation would parse value caps to determine actual axis/button counts
    m_axisCount = qMin(static_cast<int>(caps.NumberInputValueCaps), Controllers::MAX_AXES_PER_CONTROLLER);
    m_buttonCount = qMin(static_cast<int>(caps.NumberInputButtonCaps), Controllers::MAX_BUTTONS_PER_CONTROLLER);
    m_povCount = 0; // Would need to be determined from usage pages
    
    return true;
    
#elif defined(Q_OS_LINUX)
    // Get supported events
    unsigned long evBits[EV_CNT / (8 * sizeof(unsigned long)) + 1];
    if (ioctl(m_deviceFd, EVIOCGBIT(0, sizeof(evBits)), evBits) < 0) {
        return false;
    }
    
    // Check for absolute axes
    if (evBits[EV_ABS / (8 * sizeof(unsigned long))] & (1UL << (EV_ABS % (8 * sizeof(unsigned long))))) {
        unsigned long absBits[ABS_CNT / (8 * sizeof(unsigned long)) + 1];
        if (ioctl(m_deviceFd, EVIOCGBIT(EV_ABS, sizeof(absBits)), absBits) >= 0) {
            m_axisCount = 0;
            for (int i = 0; i < ABS_CNT && m_axisCount < Controllers::MAX_AXES_PER_CONTROLLER; i++) {
                if (absBits[i / (8 * sizeof(unsigned long))] & (1UL << (i % (8 * sizeof(unsigned long))))) {
                    m_axisTypes.append(i);
                    
                    // Get axis info
                    if (ioctl(m_deviceFd, EVIOCGABS(i), &m_axisInfo[i]) >= 0) {
                        m_axisCount++;
                    }
                }
            }
        }
    }
    
    // Check for buttons
    if (evBits[EV_KEY / (8 * sizeof(unsigned long))] & (1UL << (EV_KEY % (8 * sizeof(unsigned long))))) {
        unsigned long keyBits[KEY_CNT / (8 * sizeof(unsigned long)) + 1];
        if (ioctl(m_deviceFd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) >= 0) {
            m_buttonCount = 0;
            for (int i = BTN_MISC; i < KEY_CNT && m_buttonCount < Controllers::MAX_BUTTONS_PER_CONTROLLER; i++) {
                if (keyBits[i / (8 * sizeof(unsigned long))] & (1UL << (i % (8 * sizeof(unsigned long))))) {
                    m_buttonTypes.append(i);
                    m_buttonCount++;
                }
            }
        }
    }
    
    m_povCount = 0; // Linux doesn't have a standard POV representation
    
    return m_axisCount > 0 || m_buttonCount > 0;
    
#elif defined(Q_OS_MACOS)
    // Get all elements
    CFArrayRef elements = IOHIDDeviceCopyMatchingElements(m_hidDevice, NULL, kIOHIDOptionsTypeNone);
    if (!elements) {
        return false;
    }
    
    CFIndex elementCount = CFArrayGetCount(elements);
    m_axisCount = 0;
    m_buttonCount = 0;
    m_povCount = 0;
    
    for (CFIndex i = 0; i < elementCount; i++) {
        IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
        IOHIDElementType elementType = IOHIDElementGetType(element);
        uint32_t usagePage = IOHIDElementGetUsagePage(element);
        uint32_t usage = IOHIDElementGetUsage(element);
        
        if (elementType == kIOHIDElementTypeInput_Misc || elementType == kIOHIDElementTypeInput_Axis) {
            if (usagePage == kHIDPage_GenericDesktop) {
                if ((usage >= kHIDUsage_GD_X && usage <= kHIDUsage_GD_Rz) && 
                    m_axisCount < Controllers::MAX_AXES_PER_CONTROLLER) {
                    m_axisElements.append(element);
                    CFRetain(element);
                    m_axisCount++;
                } else if (usage == kHIDUsage_GD_Hatswitch && 
                          m_povCount < Controllers::MAX_POVS_PER_CONTROLLER) {
                    m_povElements.append(element);
                    CFRetain(element);
                    m_povCount++;
                }
            }
        } else if (elementType == kIOHIDElementTypeInput_Button) {
            if (usagePage == kHIDPage_Button && m_buttonCount < Controllers::MAX_BUTTONS_PER_CONTROLLER) {
                m_buttonElements.append(element);
                CFRetain(element);
                m_buttonCount++;
            }
        }
    }
    
    CFRelease(elements);
    
    return m_axisCount > 0 || m_buttonCount > 0;
#endif
    
    return false;
}

void ControllerHIDDevice::normalizeAxisValue(int axis, int rawValue)
{
    if (axis < 0 || axis >= m_axisValues.size()) {
        return;
    }
    
#ifdef Q_OS_LINUX
    if (axis < m_axisTypes.size()) {
        int axisType = m_axisTypes[axis];
        const struct input_absinfo &info = m_axisInfo[axisType];
        
        // Normalize to -1.0 to 1.0 range
        float normalizedValue = 0.0f;
        if (info.maximum > info.minimum) {
            normalizedValue = (2.0f * (rawValue - info.minimum) / (info.maximum - info.minimum)) - 1.0f;
        }
        
        m_axisValues[axis] = qBound(-1.0f, normalizedValue, 1.0f);
    }
#else
    Q_UNUSED(rawValue)
    // Platform-specific normalization handled in updateData()
#endif
}
