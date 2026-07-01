#pragma once
#include <cstdint>

// Key codes - subset of Linux evdev KEY_* values.
// Using explicit values so they match evdev without including linux/input.h
// in application code.
enum class KeyCode : uint16_t {
    Unknown = 0,
    W       = 17,
    A       = 30,
    S       = 31,
    D       = 32,
    Q       = 16,
    E       = 18,
    R       = 19,
    Tab     = 15,
    Escape  = 1,
    Enter   = 28,
    Space   = 57,
    Up      = 103,
    Down    = 108,
    Left    = 105,
    Right   = 106,
};

enum class KeyAction : uint8_t {
    Released = 0,
    Pressed  = 1,
    Repeated = 2,
};

enum class MouseButton : uint8_t {
    Left   = 0,
    Right  = 1,
    Middle = 2,
    Unknown = 255,
};

enum class MouseButtonAction : uint8_t {
    Released = 0,
    Pressed  = 1,
};

struct KeyEvent {
    KeyCode   code;
    KeyAction action;
};

struct MouseButtonEvent {
    MouseButton       button;
    MouseButtonAction action;
};

// Relative mouse movement (evdev REL_X / REL_Y)
struct MouseMoveEvent {
    int32_t dx;
    int32_t dy;
};

// Scroll wheel (evdev REL_WHEEL)
struct MouseScrollEvent {
    int32_t delta; // positive = scroll up, negative = scroll down
};

enum class InputEventType : uint8_t {
    Key,
    MouseButton,
    MouseMove,
    MouseScroll,
};

// Tagged union - avoids heap allocation per event
struct InputEvent {
    InputEventType type;
    union {
        KeyEvent         key;
        MouseButtonEvent mouse_button;
        MouseMoveEvent   mouse_move;
        MouseScrollEvent mouse_scroll;
    };

    static InputEvent make_key(KeyCode code, KeyAction action) {
        InputEvent e;
        e.type       = InputEventType::Key;
        e.key.code   = code;
        e.key.action = action;
        return e;
    }

    static InputEvent make_mouse_button(MouseButton btn, MouseButtonAction action) {
        InputEvent e;
        e.type                = InputEventType::MouseButton;
        e.mouse_button.button = btn;
        e.mouse_button.action = action;
        return e;
    }

    static InputEvent make_mouse_move(int32_t dx, int32_t dy) {
        InputEvent e;
        e.type           = InputEventType::MouseMove;
        e.mouse_move.dx  = dx;
        e.mouse_move.dy  = dy;
        return e;
    }

    static InputEvent make_mouse_scroll(int32_t delta) {
        InputEvent e;
        e.type              = InputEventType::MouseScroll;
        e.mouse_scroll.delta = delta;
        return e;
    }
};