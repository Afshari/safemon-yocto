#include <gtest/gtest.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include "evdev_mouse.h"

// ---------------------------------------------------------------------------
// EvdevMouseInjected - same injection pattern as EvdevKeyboardInjected.
// Bypasses open() and injects an already-open fd into m_fd.
// ---------------------------------------------------------------------------

class EvdevMouseInjected : public EvdevMouse {
public:
    explicit EvdevMouseInjected(int fd)
        : EvdevMouse("/dev/null/injected")
    {
        m_fd = fd;
    }

    bool open() override { return m_fd >= 0; }

    void close() override {
        // Test fixture owns the fd - just clear our reference.
        m_fd = -1;
    }
};

// ---------------------------------------------------------------------------
// Helper: write a synthetic input_event
// ---------------------------------------------------------------------------

static void write_ev(int fd, uint16_t type, uint16_t code, int32_t value)
{
    struct input_event ev{};
    ev.type  = type;
    ev.code  = code;
    ev.value = value;
    ::write(fd, &ev, sizeof(ev));
}

static void write_syn(int fd)
{
    write_ev(fd, EV_SYN, SYN_REPORT, 0);
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class EvdevMouseTest : public ::testing::Test {
protected:
    int sv[2] = {-1, -1};

    void SetUp() override {
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
        int flags = fcntl(sv[1], F_GETFL, 0);
        ASSERT_GE(flags, 0);
        ASSERT_EQ(fcntl(sv[1], F_SETFL, flags | O_NONBLOCK), 0);
    }

    void TearDown() override {
        if (sv[0] >= 0) ::close(sv[0]);
        if (sv[1] >= 0) ::close(sv[1]);
    }

    std::unique_ptr<EvdevMouseInjected> make_mouse() {
        return std::make_unique<EvdevMouseInjected>(sv[1]);
    }
};

// ---------------------------------------------------------------------------
// translate_button tests (pure logic, no fd)
// ---------------------------------------------------------------------------

TEST(EvdevMouseTranslate, MapsKnownButtons)
{
    EXPECT_EQ(EvdevMouse::translate_button(BTN_LEFT),   MouseButton::Left);
    EXPECT_EQ(EvdevMouse::translate_button(BTN_RIGHT),  MouseButton::Right);
    EXPECT_EQ(EvdevMouse::translate_button(BTN_MIDDLE), MouseButton::Middle);
}

TEST(EvdevMouseTranslate, UnmappedButtonReturnsUnknown)
{
    EXPECT_EQ(EvdevMouse::translate_button(BTN_SIDE),   MouseButton::Unknown);
    EXPECT_EQ(EvdevMouse::translate_button(0xFFFF),     MouseButton::Unknown);
}

// ---------------------------------------------------------------------------
// Open / close / is_open
// ---------------------------------------------------------------------------

TEST_F(EvdevMouseTest, IsOpenAfterInjection)
{
    auto m = make_mouse();
    EXPECT_TRUE(m->is_open());
}

TEST_F(EvdevMouseTest, IsClosedAfterClose)
{
    auto m = make_mouse();
    m->close();
    EXPECT_FALSE(m->is_open());
}

TEST(EvdevMouseOpen, RealOpenFailsOnBadPath)
{
    EvdevMouse m("/dev/input/this_does_not_exist");
    EXPECT_FALSE(m.open());
    EXPECT_FALSE(m.is_open());
}

TEST_F(EvdevMouseTest, DoubleCloseIsSafe)
{
    auto m = make_mouse();
    m->close();
    m->close();
    EXPECT_FALSE(m->is_open());
}

// ---------------------------------------------------------------------------
// poll() - no events
// ---------------------------------------------------------------------------

TEST_F(EvdevMouseTest, PollEmptyReturnsNoEvents)
{
    auto m = make_mouse();
    std::vector<InputEvent> events;
    EXPECT_TRUE(m->poll(events));
    EXPECT_TRUE(events.empty());
}

TEST_F(EvdevMouseTest, PollReturnsFalseWhenClosed)
{
    auto m = make_mouse();
    m->close();
    std::vector<InputEvent> events;
    EXPECT_FALSE(m->poll(events));
    EXPECT_TRUE(events.empty());
}

// ---------------------------------------------------------------------------
// poll() - mouse movement
// ---------------------------------------------------------------------------

TEST_F(EvdevMouseTest, PollDeliversMoveAfterSyn)
{
    auto m = make_mouse();
    write_ev(sv[0], EV_REL, REL_X, 10);
    write_ev(sv[0], EV_REL, REL_Y, -5);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type,          InputEventType::MouseMove);
    EXPECT_EQ(events[0].mouse_move.dx, 10);
    EXPECT_EQ(events[0].mouse_move.dy, -5);
}

TEST_F(EvdevMouseTest, PollAccumulatesXYWithinFrame)
{
    // Two REL_X events in one frame - should be summed into one MouseMove.
    auto m = make_mouse();
    write_ev(sv[0], EV_REL, REL_X, 3);
    write_ev(sv[0], EV_REL, REL_X, 7);
    write_ev(sv[0], EV_REL, REL_Y, 2);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].mouse_move.dx, 10); // 3 + 7
    EXPECT_EQ(events[0].mouse_move.dy, 2);
}

TEST_F(EvdevMouseTest, PollEmitsSeparateMovePerSynFrame)
{
    auto m = make_mouse();
    write_ev(sv[0], EV_REL, REL_X, 5);
    write_syn(sv[0]);
    write_ev(sv[0], EV_REL, REL_X, 3);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].mouse_move.dx, 5);
    EXPECT_EQ(events[1].mouse_move.dx, 3);
}

TEST_F(EvdevMouseTest, PollFlushesPartialMoveOnEAGAIN)
{
    // Motion without a trailing SYN - should still be flushed when EAGAIN hit.
    auto m = make_mouse();
    write_ev(sv[0], EV_REL, REL_X, 4);
    write_ev(sv[0], EV_REL, REL_Y, 6);
    // No SYN written - poll() will hit EAGAIN and flush.

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].mouse_move.dx, 4);
    EXPECT_EQ(events[0].mouse_move.dy, 6);
}

TEST_F(EvdevMouseTest, PollNoMoveEventIfNeitherAxisMoved)
{
    // A SYN with no preceding REL events should not emit a zero-delta move.
    auto m = make_mouse();
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    EXPECT_TRUE(events.empty());
}

// ---------------------------------------------------------------------------
// poll() - scroll wheel
// ---------------------------------------------------------------------------

TEST_F(EvdevMouseTest, PollDeliversScrollUp)
{
    auto m = make_mouse();
    write_ev(sv[0], EV_REL, REL_WHEEL, 1);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type,               InputEventType::MouseScroll);
    EXPECT_EQ(events[0].mouse_scroll.delta, 1);
}

TEST_F(EvdevMouseTest, PollDeliversScrollDown)
{
    auto m = make_mouse();
    write_ev(sv[0], EV_REL, REL_WHEEL, -1);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].mouse_scroll.delta, -1);
}

TEST_F(EvdevMouseTest, PollAccumulatesScrollWithinFrame)
{
    auto m = make_mouse();
    write_ev(sv[0], EV_REL, REL_WHEEL, 1);
    write_ev(sv[0], EV_REL, REL_WHEEL, 1);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].mouse_scroll.delta, 2);
}

// ---------------------------------------------------------------------------
// poll() - buttons
// ---------------------------------------------------------------------------

TEST_F(EvdevMouseTest, PollDeliversLeftButtonPress)
{
    auto m = make_mouse();
    write_ev(sv[0], EV_KEY, BTN_LEFT, 1);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type,                InputEventType::MouseButton);
    EXPECT_EQ(events[0].mouse_button.button, MouseButton::Left);
    EXPECT_EQ(events[0].mouse_button.action, MouseButtonAction::Pressed);
}

TEST_F(EvdevMouseTest, PollDeliversRightButtonRelease)
{
    auto m = make_mouse();
    write_ev(sv[0], EV_KEY, BTN_RIGHT, 0);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].mouse_button.button, MouseButton::Right);
    EXPECT_EQ(events[0].mouse_button.action, MouseButtonAction::Released);
}

TEST_F(EvdevMouseTest, PollDeliversMiddleButton)
{
    auto m = make_mouse();
    write_ev(sv[0], EV_KEY, BTN_MIDDLE, 1);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].mouse_button.button, MouseButton::Middle);
}

TEST_F(EvdevMouseTest, PollDeliversUnknownButton)
{
    auto m = make_mouse();
    write_ev(sv[0], EV_KEY, BTN_SIDE, 1);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].mouse_button.button, MouseButton::Unknown);
}

// ---------------------------------------------------------------------------
// poll() - combined events in one frame
// ---------------------------------------------------------------------------

TEST_F(EvdevMouseTest, PollMoveAndButtonInSameFrame)
{
    // Button events flush pending motion first, so the order should be:
    // move (from REL before button), then button.
    auto m = make_mouse();
    write_ev(sv[0], EV_REL, REL_X,   8);
    write_ev(sv[0], EV_KEY, BTN_LEFT, 1);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].type,          InputEventType::MouseMove);
    EXPECT_EQ(events[0].mouse_move.dx, 8);
    EXPECT_EQ(events[1].type,          InputEventType::MouseButton);
    EXPECT_EQ(events[1].mouse_button.button, MouseButton::Left);
}

TEST_F(EvdevMouseTest, PollMoveAndScrollInSameFrame)
{
    auto m = make_mouse();
    write_ev(sv[0], EV_REL, REL_X,     2);
    write_ev(sv[0], EV_REL, REL_Y,     3);
    write_ev(sv[0], EV_REL, REL_WHEEL, 1);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].type, InputEventType::MouseMove);
    EXPECT_EQ(events[1].type, InputEventType::MouseScroll);
}

TEST_F(EvdevMouseTest, PollMultipleFrames)
{
    auto m = make_mouse();
    // Frame 1: move
    write_ev(sv[0], EV_REL, REL_X, 1);
    write_syn(sv[0]);
    // Frame 2: button
    write_ev(sv[0], EV_KEY, BTN_RIGHT, 1);
    write_syn(sv[0]);
    // Frame 3: scroll
    write_ev(sv[0], EV_REL, REL_WHEEL, -1);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].type, InputEventType::MouseMove);
    EXPECT_EQ(events[1].type, InputEventType::MouseButton);
    EXPECT_EQ(events[2].type, InputEventType::MouseScroll);
}

TEST_F(EvdevMouseTest, PollIgnoresHorizontalScroll)
{
    // REL_HWHEEL should be silently ignored for now.
    auto m = make_mouse();
    write_ev(sv[0], EV_REL, REL_HWHEEL, 1);
    write_syn(sv[0]);

    std::vector<InputEvent> events;
    ASSERT_TRUE(m->poll(events));
    EXPECT_TRUE(events.empty());
}

TEST_F(EvdevMouseTest, PollAppendsToExistingVector)
{
    auto m = make_mouse();
    std::vector<InputEvent> events;
    events.push_back(InputEvent::make_key(KeyCode::Q, KeyAction::Pressed));

    write_ev(sv[0], EV_REL, REL_X, 5);
    write_syn(sv[0]);

    ASSERT_TRUE(m->poll(events));
    ASSERT_EQ(events.size(), 2u);
    // Original event at index 0 is preserved
    EXPECT_EQ(events[0].type,         InputEventType::Key);
    EXPECT_EQ(events[0].key.code,     KeyCode::Q);
    // New mouse move appended at index 1
    EXPECT_EQ(events[1].type,         InputEventType::MouseMove);
    EXPECT_EQ(events[1].mouse_move.dx, 5);
}