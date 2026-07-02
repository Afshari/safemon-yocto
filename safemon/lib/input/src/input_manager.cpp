#include "input_manager.h"
#include <iostream>

InputManager::InputManager(std::unique_ptr<IInputDevice> keyboard,
                           std::unique_ptr<IInputDevice> mouse)
    : m_keyboard(std::move(keyboard))
    , m_mouse(std::move(mouse))
{}

InputManager::~InputManager()
{
    close();
}

bool InputManager::open()
{
    if (m_keyboard) {
        m_keyboard_ok = m_keyboard->open();
        if (!m_keyboard_ok) {
            std::cerr << "[input_manager] Keyboard unavailable\n";
        }
    }

    if (m_mouse) {
        m_mouse_ok = m_mouse->open();
        if (!m_mouse_ok) {
            std::cerr << "[input_manager] Mouse unavailable\n";
        }
    }

    bool any_open = m_keyboard_ok || m_mouse_ok;
    if (!any_open) {
        std::cerr << "[input_manager] No input devices available\n";
    }
    return any_open;
}

void InputManager::close()
{
    if (m_keyboard) {
        m_keyboard->close();
        m_keyboard_ok = false;
    }
    if (m_mouse) {
        m_mouse->close();
        m_mouse_ok = false;
    }
}

size_t InputManager::poll(std::vector<InputEvent>& out)
{
    size_t before = out.size();

    if (m_keyboard_ok) {
        bool ok = m_keyboard->poll(out);
        if (!ok) {
            std::cerr << "[input_manager] Keyboard disconnected\n";
            m_keyboard_ok = false;
        }
    }

    if (m_mouse_ok) {
        bool ok = m_mouse->poll(out);
        if (!ok) {
            std::cerr << "[input_manager] Mouse disconnected\n";
            m_mouse_ok = false;
        }
    }

    return out.size() - before;
}

std::vector<InputEvent> InputManager::poll()
{
    std::vector<InputEvent> events;
    poll(events);
    return events;
}

bool InputManager::reopen()
{
    if (m_keyboard && !m_keyboard_ok) {
        m_keyboard_ok = m_keyboard->open();
        if (m_keyboard_ok) {
            std::cout << "[input_manager] Keyboard reconnected\n";
        }
    }

    if (m_mouse && !m_mouse_ok) {
        m_mouse_ok = m_mouse->open();
        if (m_mouse_ok) {
            std::cout << "[input_manager] Mouse reconnected\n";
        }
    }

    return m_keyboard_ok || m_mouse_ok;
}

bool InputManager::keyboard_available() const { return m_keyboard_ok; }
bool InputManager::mouse_available()    const { return m_mouse_ok;    }