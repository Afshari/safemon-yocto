#include "evdev_mouse.h"
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <iostream>

EvdevMouse::EvdevMouse(std::string device_path)
    : m_path(std::move(device_path))
{}

EvdevMouse::~EvdevMouse()
{
    close();
}

bool EvdevMouse::open()
{
    if (m_fd >= 0) return true; // already open

    m_fd = ::open(m_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (m_fd < 0) {
        std::cerr << "[evdev_mouse] Failed to open " << m_path << "\n";
        return false;
    }
    std::cout << "[evdev_mouse] Opened " << m_path << "\n";
    return true;
}

void EvdevMouse::close()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
        std::cout << "[evdev_mouse] Closed " << m_path << "\n";
    }
}

bool EvdevMouse::is_open() const
{
    return m_fd >= 0;
}

bool EvdevMouse::poll(std::vector<InputEvent>& out)
{
    if (m_fd < 0) return false;

    struct input_event ev;
    while (true) {
        ssize_t n = read(m_fd, &ev, sizeof(ev));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more events - flush any accumulated motion and return.
                flush_pending(out);
                return true;
            }
            // Real error - device likely disconnected.
            std::cerr << "[evdev_mouse] Read error on " << m_path << "\n";
            flush_pending(out);
            close();
            return false;
        }
        if (n < static_cast<ssize_t>(sizeof(ev))) continue;

        switch (ev.type) {
            case EV_SYN:
                // End of a logical event frame - flush accumulated motion.
                flush_pending(out);
                break;

            case EV_REL:
                switch (ev.code) {
                    case REL_X:
                        m_pending_dx += ev.value;
                        break;
                    case REL_Y:
                        m_pending_dy += ev.value;
                        break;
                    case REL_WHEEL:
                        m_pending_scroll += ev.value;
                        break;
                    default:
                        break; // REL_HWHEEL etc - ignored for now
                }
                break;

            case EV_KEY: {
                // Mouse button event - flush any pending motion first so
                // button events appear in the correct order relative to moves.
                flush_pending(out);
                MouseButton btn = translate_button(ev.code);
                MouseButtonAction action = (ev.value == 1)
                    ? MouseButtonAction::Pressed
                    : MouseButtonAction::Released;
                out.push_back(InputEvent::make_mouse_button(btn, action));
                break;
            }

            default:
                break; // EV_MSC etc - ignored
        }
    }
}

void EvdevMouse::flush_pending(std::vector<InputEvent>& out)
{
    if (m_pending_dx != 0 || m_pending_dy != 0) {
        out.push_back(InputEvent::make_mouse_move(m_pending_dx, m_pending_dy));
        m_pending_dx = 0;
        m_pending_dy = 0;
    }
    if (m_pending_scroll != 0) {
        out.push_back(InputEvent::make_mouse_scroll(m_pending_scroll));
        m_pending_scroll = 0;
    }
}

MouseButton EvdevMouse::translate_button(uint16_t evdev_code)
{
    switch (evdev_code) {
        case BTN_LEFT:   return MouseButton::Left;
        case BTN_RIGHT:  return MouseButton::Right;
        case BTN_MIDDLE: return MouseButton::Middle;
        default:         return MouseButton::Unknown;
    }
}