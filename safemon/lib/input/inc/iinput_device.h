#pragma once
#include <vector>
#include "input_event_types.h"

// Interface for any input device (evdev keyboard, evdev mouse, Wayland seat).
// poll() is non-blocking - it reads all pending events and appends them to
// the provided vector. Returns false if the device has become unavailable
// (e.g. unplugged or fd closed).
class IInputDevice {
public:
    virtual ~IInputDevice() = default;

    // Open the device. Returns true on success.
    virtual bool open()  = 0;

    // Close the device and release the file descriptor.
    virtual void close() = 0;

    // Returns true if the device is currently open.
    virtual bool is_open() const = 0;

    // Non-blocking read. Appends any pending events to out.
    // Returns false if the device has become unavailable.
    virtual bool poll(std::vector<InputEvent>& out) = 0;
};