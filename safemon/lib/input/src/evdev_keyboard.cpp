#include "evdev_keyboard.h"
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <iostream>

EvdevKeyboard::EvdevKeyboard(std::string device_path)
    : m_path(std::move(device_path))
{}

EvdevKeyboard::~EvdevKeyboard()
{
    close();
}

bool EvdevKeyboard::open()
{
    if (m_fd >= 0) return true; // already open

    m_fd = ::open(m_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (m_fd < 0) {
        std::cerr << "[evdev_keyboard] Failed to open " << m_path << "\n";
        return false;
    }
    std::cout << "[evdev_keyboard] Opened " << m_path << "\n";
    return true;
}

void EvdevKeyboard::close()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
        std::cout << "[evdev_keyboard] Closed " << m_path << "\n";
    }
}

bool EvdevKeyboard::is_open() const
{
    return m_fd >= 0;
}

bool EvdevKeyboard::poll(std::vector<InputEvent>& out)
{
    if (m_fd < 0) return false;

    struct input_event ev;
    while (true) {
        ssize_t n = read(m_fd, &ev, sizeof(ev));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more events - normal non-blocking return
                return true;
            }
            // Real error - device likely disconnected
            std::cerr << "[evdev_keyboard] Read error on " << m_path << "\n";
            close();
            return false;
        }
        if (n < static_cast<ssize_t>(sizeof(ev))) continue;

        // Only handle EV_KEY events
        if (ev.type != EV_KEY) continue;

        KeyCode code = translate_key(ev.code);
        // Still emit Unknown keys so the app can see raw activity if needed,
        // but most apps will filter Unknown out.
        KeyAction action;
        switch (ev.value) {
            case 0:  action = KeyAction::Released; break;
            case 1:  action = KeyAction::Pressed;  break;
            case 2:  action = KeyAction::Repeated; break;
            default: continue; // unexpected value
        }

        out.push_back(InputEvent::make_key(code, action));
    }
}

KeyCode EvdevKeyboard::translate_key(uint16_t evdev_code)
{
    // evdev key codes match our KeyCode enum values directly (defined to match).
    // Cast is safe - unmapped codes fall through to Unknown.
    switch (evdev_code) {
        case 17:  return KeyCode::W;
        case 30:  return KeyCode::A;
        case 31:  return KeyCode::S;
        case 32:  return KeyCode::D;
        case 16:  return KeyCode::Q;
        case 18:  return KeyCode::E;
        case 19:  return KeyCode::R;
        case 15:  return KeyCode::Tab;
        case 1:   return KeyCode::Escape;
        case 28:  return KeyCode::Enter;
        case 57:  return KeyCode::Space;
        case 103: return KeyCode::Up;
        case 108: return KeyCode::Down;
        case 105: return KeyCode::Left;
        case 106: return KeyCode::Right;
        default:  return KeyCode::Unknown;
    }
}