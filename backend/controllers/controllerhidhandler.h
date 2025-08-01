#ifndef CONTROLLERHIDHANDLER_H
#define CONTROLLERHIDHANDLER_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QList>
#include <memory>
#include "controllerhiddevice.h"

namespace FRCDriverStation {

class Logger;

/**
 * @brief Manages HID controller detection, binding, and data collection
 * 
 * This class handles:
 * - Automatic detection of connected HID controllers
 * - Binding controllers to specific slots (0-5)
 * - Polling controller data at regular intervals
 * - Managing controller connection/disconnection events
 * 
 * Design principles:
 * - Single responsibility: Only handles controller management
 * - Observable: Emits signals for controller events
 * - Fail gracefully: Handles controller disconnections cleanly
 * - Platform abstraction: Hides platform-specific HID details
 */
class ControllerHIDHandler : public QObject
{
    Q_OBJECT

public:
    static constexpr int MAX_CONTROLLER_SLOTS = 6;

    explicit ControllerHIDHandler(std::shared_ptr<Logger> logger, QObject *parent = nullptr);
    ~ControllerHIDHandler();

    // Controller management
    void startPolling();
    void stopPolling();
    void refreshControllers();
    
    // Slot management
    bool bindControllerToSlot(const QString &deviceId, int slot);
    void unbindControllerFromSlot(int slot);
    ControllerHIDDevice* getControllerInSlot(int slot) const;
    int getSlotForController(const QString &deviceId) const;
    
    // Controller queries
    QList<ControllerHIDDevice*> getAllControllers() const;
    QList<ControllerHIDDevice*> getAllBoundControllers() const;
    QList<ControllerHIDDevice*> getUnboundControllers() const;
    ControllerHIDDevice* getControllerById(const QString &deviceId) const;
    
    // Status queries
    bool isPolling() const { return m_polling; }
    int getBoundControllerCount() const;

signals:
    void controllerConnected(ControllerHIDDevice *controller);
    void controllerDisconnected(const QString &deviceId);
    void controllerBound(ControllerHIDDevice *controller, int slot);
    void controllerUnbound(const QString &deviceId, int slot);
    void controllerDataChanged(int slot);

private slots:
    void pollControllers();
    void detectControllers();

private:
    void addController(ControllerHIDDevice *controller);
    void removeController(const QString &deviceId);
    void updateControllerData();
    QList<ControllerHIDDevice*> enumerateHIDDevices();

    std::shared_ptr<Logger> m_logger;
    std::unique_ptr<QTimer> m_pollTimer;
    std::unique_ptr<QTimer> m_detectTimer;
    
    // Controller storage
    QMap<QString, std::unique_ptr<ControllerHIDDevice>> m_controllers;
    QMap<int, ControllerHIDDevice*> m_slotBindings; // slot -> controller mapping
    
    bool m_polling;
};

} // namespace FRCDriverStation

#endif // CONTROLLERHIDHANDLER_H
