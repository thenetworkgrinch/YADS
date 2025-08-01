#ifndef CONTROLLERHIDDEVICE_H
#define CONTROLLERHIDDEVICE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <linux/input.h>
#elif defined(Q_OS_MACOS)
#include <IOKit/hid/IOHIDDevice.h>
#endif

namespace FRCDriverStation {

class Logger;

/**
 * @brief Represents a single HID controller device
 * 
 * This class encapsulates a physical controller and provides:
 * - Platform-specific HID communication
 * - Axis, button, and POV data reading
 * - Device identification and capabilities
 * - Connection state management
 * 
 * Design principles:
 * - Platform abstraction: Hides OS-specific HID details
 * - Immutable identity: Device ID never changes once created
 * - Fail gracefully: Handles device disconnection cleanly
 * - Efficient polling: Minimizes system calls during data updates
 */
class ControllerHIDDevice : public QObject
{
    Q_OBJECT

public:
    ~ControllerHIDDevice();

    // Factory methods for platform-specific creation
    static ControllerHIDDevice* createFromPath(const QString &devicePath, std::shared_ptr<Logger> logger);
    
#ifdef Q_OS_MACOS
    static ControllerHIDDevice* createFromIOHIDDevice(IOHIDDeviceRef device, std::shared_ptr<Logger> logger);
#endif

    // Device identification
    QString deviceId() const { return m_deviceId; }
    QString name() const { return m_name; }
    quint16 vendorId() const { return m_vendorId; }
    quint16 productId() const { return m_productId; }
    
    // Connection state
    bool isConnected() const { return m_connected; }
    bool isGameController() const;
    
    // Capabilities
    int getAxisCount() const { return m_axisCount; }
    int getButtonCount() const { return m_buttonCount; }
    int getPOVCount() const { return m_povCount; }
    
    // Data access
    float getAxisValue(int axis) const;
    bool getButtonValue(int button) const;
    qint16 getPOVValue(int pov) const;
    
    // Data update
    bool updateData();

signals:
    void disconnected();

private:
    explicit ControllerHIDDevice(const QString &devicePath, std::shared_ptr<Logger> logger, QObject *parent = nullptr);
    
    bool openDevice();
    void closeDevice();
    bool readDeviceInfo();
    bool readCapabilities();
    void normalizeAxisValue(int axis, int rawValue);
    
    std::shared_ptr<Logger> m_logger;
    QString m_devicePath;
    QString m_deviceId;
    QString m_name;
    quint16 m_vendorId;
    quint16 m_productId;
    bool m_connected;
    
    // Capabilities
    int m_axisCount;
    int m_buttonCount;
    int m_povCount;
    
    // Current state
    QVector<float> m_axisValues;
    QVector<bool> m_buttonValues;
    QVector<qint16> m_povValues;
    
    // Platform-specific data
#ifdef Q_OS_WIN
    HANDLE m_deviceHandle;
    struct _HIDP_PREPARSED_DATA* m_preparsedData;
    QVector<int> m_axisUsages;
    QVector<int> m_buttonUsages;
#elif defined(Q_OS_LINUX)
    int m_deviceFd;
    QVector<int> m_axisTypes;
    QVector<int> m_buttonTypes;
    struct input_absinfo m_axisInfo[ABS_CNT];
#elif defined(Q_OS_MACOS)
    IOHIDDeviceRef m_hidDevice;
    QVector<IOHIDElementRef> m_axisElements;
    QVector<IOHIDElementRef> m_buttonElements;
    QVector<IOHIDElementRef> m_povElements;
#endif
};

} // namespace FRCDriverStation

#endif // CONTROLLERHIDDEVICE_H
