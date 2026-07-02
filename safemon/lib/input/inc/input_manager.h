#pragma once
#include <memory>
#include <vector>
#include <string>
#include "iinput_device.h"
#include "input_event_types.h"

// InputManager owns one keyboard and one mouse IInputDevice.
// Call poll() once per render frame to get all pending events from both
// devices merged into a single vector, in device order (keyboard first,
// then mouse).
//
// Devices are optional - passing nullptr for either is valid and that
// device is simply skipped during poll().
//
// If a device's poll() returns false (device disconnected), InputManager
// marks it unavailable and stops polling it. The device is not
// automatically re-opened - call reopen() to attempt reconnection.
class InputManager {
public:
    // Takes ownership of the provided devices. Either may be nullptr.
    InputManager(std::unique_ptr<IInputDevice> keyboard,
                 std::unique_ptr<IInputDevice> mouse);

    ~InputManager();

    // Open both devices. Logs warnings for any that fail to open but
    // does not abort - partial availability is acceptable.
    // Returns true if at least one device opened successfully.
    bool open();

    // Close both devices.
    void close();

    // Poll all available devices and append their events to out.
    // Should be called once per render frame.
    // Returns the number of events appended.
    size_t poll(std::vector<InputEvent>& out);

    // Convenience overload - returns a freshly populated vector.
    std::vector<InputEvent> poll();

    // Attempt to reopen any device that is currently not open.
    // Useful after a disconnect. Returns true if at least one device
    // is now open after the attempt.
    bool reopen();

    bool keyboard_available() const;
    bool mouse_available()    const;

private:
    std::unique_ptr<IInputDevice> m_keyboard;
    std::unique_ptr<IInputDevice> m_mouse;

    bool m_keyboard_ok = false; // false if device is nullptr or disconnected
    bool m_mouse_ok    = false;
};