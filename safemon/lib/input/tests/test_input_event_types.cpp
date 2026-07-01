#include <gtest/gtest.h>
#include "input_event_types.h"

// ---------------------------------------------------------------------------
// InputEvent factory methods
// ---------------------------------------------------------------------------

TEST(InputEventTypes, MakeKeyPressed)
{
    auto e = InputEvent::make_key(KeyCode::W, KeyAction::Pressed);
    EXPECT_EQ(e.type,       InputEventType::Key);
    EXPECT_EQ(e.key.code,   KeyCode::W);
    EXPECT_EQ(e.key.action, KeyAction::Pressed);
}

TEST(InputEventTypes, MakeKeyReleased)
{
    auto e = InputEvent::make_key(KeyCode::Escape, KeyAction::Released);
    EXPECT_EQ(e.type,       InputEventType::Key);
    EXPECT_EQ(e.key.code,   KeyCode::Escape);
    EXPECT_EQ(e.key.action, KeyAction::Released);
}

TEST(InputEventTypes, MakeKeyRepeated)
{
    auto e = InputEvent::make_key(KeyCode::Space, KeyAction::Repeated);
    EXPECT_EQ(e.key.action, KeyAction::Repeated);
}

TEST(InputEventTypes, MakeMouseButtonLeft)
{
    auto e = InputEvent::make_mouse_button(MouseButton::Left, MouseButtonAction::Pressed);
    EXPECT_EQ(e.type,                InputEventType::MouseButton);
    EXPECT_EQ(e.mouse_button.button, MouseButton::Left);
    EXPECT_EQ(e.mouse_button.action, MouseButtonAction::Pressed);
}

TEST(InputEventTypes, MakeMouseButtonRight)
{
    auto e = InputEvent::make_mouse_button(MouseButton::Right, MouseButtonAction::Released);
    EXPECT_EQ(e.mouse_button.button, MouseButton::Right);
    EXPECT_EQ(e.mouse_button.action, MouseButtonAction::Released);
}

TEST(InputEventTypes, MakeMouseMovePositive)
{
    auto e = InputEvent::make_mouse_move(10, 20);
    EXPECT_EQ(e.type,          InputEventType::MouseMove);
    EXPECT_EQ(e.mouse_move.dx, 10);
    EXPECT_EQ(e.mouse_move.dy, 20);
}

TEST(InputEventTypes, MakeMouseMoveNegative)
{
    auto e = InputEvent::make_mouse_move(-5, -15);
    EXPECT_EQ(e.mouse_move.dx, -5);
    EXPECT_EQ(e.mouse_move.dy, -15);
}

TEST(InputEventTypes, MakeMouseScrollUp)
{
    auto e = InputEvent::make_mouse_scroll(1);
    EXPECT_EQ(e.type,               InputEventType::MouseScroll);
    EXPECT_EQ(e.mouse_scroll.delta, 1);
}

TEST(InputEventTypes, MakeMouseScrollDown)
{
    auto e = InputEvent::make_mouse_scroll(-1);
    EXPECT_EQ(e.mouse_scroll.delta, -1);
}

// ---------------------------------------------------------------------------
// KeyCode enum values must match Linux evdev codes exactly
// (important for EvdevKeyboard translation correctness)
// ---------------------------------------------------------------------------

TEST(KeyCodeValues, MatchEvdevCodes)
{
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::Q),      16u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::W),      17u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::E),      18u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::R),      19u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::A),      30u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::S),      31u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::D),      32u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::Tab),    15u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::Escape),  1u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::Enter),  28u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::Space),  57u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::Up),    103u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::Down),  108u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::Left),  105u);
    EXPECT_EQ(static_cast<uint16_t>(KeyCode::Right), 106u);
}