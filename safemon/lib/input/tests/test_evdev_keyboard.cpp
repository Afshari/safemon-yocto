#include <gtest/gtest.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <cstring>

#include "evdev_keyboard.h"

// ---------------------------------------------------------------------------
// EvdevKeyboardInjected - test subclass that bypasses open() and injects an
// already-open fd directly into m_fd. This lets tests use a socketpair as
// the simulated device without needing root or real hardware.
// ---------------------------------------------------------------------------

class EvdevKeyboardInjected : public EvdevKeyboard {
public:
    explicit EvdevKeyboardInjected(int fd)
        : EvdevKeyboard("/dev/null/injected")
    {
        m_fd = fd; // inject directly - m_fd is protected in EvdevKeyboard
    }

    // Override open/close so the test controls the fd lifetime, not the base class.
    bool open() override {
        return m_fd >= 0;
    }

    void close() override {
        // Do not close the injected fd here - the test fixture owns it.
        m_fd = -1;
    }
};

// ---------------------------------------------------------------------------
// Helper: write a synthetic input_event to a fd
// ---------------------------------------------------------------------------

static void write_ev(int fd, uint16_t type, uint16_t code, int32_t value)
{
    struct input_event ev{};
    ev.type  = type;
    ev.code  = code;
    ev.value = value;
    ::write(fd, &ev, sizeof(ev));
}

// ---------------------------------------------------------------------------
// Fixture: socketpair, write end = sv[0], read end = sv[1]
// ---------------------------------------------------------------------------

class EvdevKeyboardTest : public ::testing::Test {
protected:
    int sv[2] = {-1, -1};

    void SetUp() override {
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
        // Make the read end non-blocking to match O_NONBLOCK from real open()
        int flags = fcntl(sv[1], F_GETFL, 0);
        ASSERT_GE(flags, 0);
        ASSERT_EQ(fcntl(sv[1], F_SETFL, flags | O_NONBLOCK), 0);
    }

    void TearDown() override {
        if (sv[0] >= 0) ::close(sv[0]);
        if (sv[1] >= 0) ::close(sv[1]);
    }

    // Create a keyboard with sv[1] injected as the device fd.
    std::unique_ptr<EvdevKeyboardInjected> make_keyboard() {
        return std::make_unique<EvdevKeyboardInjected>(sv[1]);
    }
};

// ---------------------------------------------------------------------------
// translate_key tests (no fd needed - pure logic)
// ---------------------------------------------------------------------------

TEST(EvdevKeyboardTranslate, MapsAllKnownCodes)
{
    struct { uint16_t evdev; KeyCode expected; } mapping[] = {
        { KEY_Q,     KeyCode::Q      },
        { KEY_W,     KeyCode::W      },
        { KEY_E,     KeyCode::E      },
        { KEY_R,     KeyCode::R      },
        { KEY_A,     KeyCode::A      },
        { KEY_S,     KeyCode::S      },
        { KEY_D,     KeyCode::D      },
        { KEY_TAB,   KeyCode::Tab    },
        { KEY_ESC,   KeyCode::Escape },
        { KEY_ENTER, KeyCode::Enter  },
        { KEY_SPACE, KeyCode::Space  },
        { KEY_UP,    KeyCode::Up     },
        { KEY_DOWN,  KeyCode::Down   },
        { KEY_LEFT,  KeyCode::Left   },
        { KEY_RIGHT, KeyCode::Right  },
    };

    for (auto& m : mapping) {
        EXPECT_EQ(EvdevKeyboard::translate_key(m.evdev), m.expected)
            << "Failed for evdev code " << m.evdev;
    }
}

TEST(EvdevKeyboardTranslate, UnmappedCodeReturnsUnknown)
{
    EXPECT_EQ(EvdevKeyboard::translate_key(999), KeyCode::Unknown);
    EXPECT_EQ(EvdevKeyboard::translate_key(0),   KeyCode::Unknown);
}

// ---------------------------------------------------------------------------
// Open / close / is_open
// ---------------------------------------------------------------------------

TEST_F(EvdevKeyboardTest, IsOpenAfterInjection)
{
    auto kb = make_keyboard();
    EXPECT_TRUE(kb->is_open());
}

TEST_F(EvdevKeyboardTest, IsClosedAfterClose)
{
    auto kb = make_keyboard();
    EXPECT_TRUE(kb->is_open());
    kb->close();
    EXPECT_FALSE(kb->is_open());
}

TEST(EvdevKeyboardOpen, RealOpenFailsOnBadPath)
{
    EvdevKeyboard kb("/dev/input/this_does_not_exist");
    EXPECT_FALSE(kb.open());
    EXPECT_FALSE(kb.is_open());
}

TEST_F(EvdevKeyboardTest, DoubleOpenIsIdempotent)
{
    auto kb = make_keyboard();
    EXPECT_TRUE(kb->open()); // already open - should return true
    EXPECT_TRUE(kb->is_open());
}

TEST_F(EvdevKeyboardTest, DoubleCloseIsSafe)
{
    auto kb = make_keyboard();
    kb->close();
    kb->close(); // second close - must not crash
    EXPECT_FALSE(kb->is_open());
}

// ---------------------------------------------------------------------------
// poll() - event delivery
// ---------------------------------------------------------------------------

TEST_F(EvdevKeyboardTest, PollEmptyReturnsNoEvents)
{
    auto kb = make_keyboard();
    std::vector<InputEvent> events;
    bool ok = kb->poll(events);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(events.empty());
}

TEST_F(EvdevKeyboardTest, PollReturnsFalseWhenClosed)
{
    auto kb = make_keyboard();
    kb->close();
    std::vector<InputEvent> events;
    EXPECT_FALSE(kb->poll(events));
    EXPECT_TRUE(events.empty());
}

TEST_F(EvdevKeyboardTest, PollDeliversSingleKeyPress)
{
    auto kb = make_keyboard();
    write_ev(sv[0], EV_KEY, KEY_W, 1);

    std::vector<InputEvent> events;
    ASSERT_TRUE(kb->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type,       InputEventType::Key);
    EXPECT_EQ(events[0].key.code,   KeyCode::W);
    EXPECT_EQ(events[0].key.action, KeyAction::Pressed);
}

TEST_F(EvdevKeyboardTest, PollDeliversSingleKeyRelease)
{
    auto kb = make_keyboard();
    write_ev(sv[0], EV_KEY, KEY_W, 0);

    std::vector<InputEvent> events;
    ASSERT_TRUE(kb->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].key.code,   KeyCode::W);
    EXPECT_EQ(events[0].key.action, KeyAction::Released);
}

TEST_F(EvdevKeyboardTest, PollDeliversSingleKeyRepeat)
{
    auto kb = make_keyboard();
    write_ev(sv[0], EV_KEY, KEY_W, 2);

    std::vector<InputEvent> events;
    ASSERT_TRUE(kb->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].key.action, KeyAction::Repeated);
}

TEST_F(EvdevKeyboardTest, PollDeliversMultipleEvents)
{
    auto kb = make_keyboard();
    write_ev(sv[0], EV_KEY, KEY_W, 1);
    write_ev(sv[0], EV_KEY, KEY_A, 1);
    write_ev(sv[0], EV_KEY, KEY_S, 0);

    std::vector<InputEvent> events;
    ASSERT_TRUE(kb->poll(events));
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].key.code,   KeyCode::W);
    EXPECT_EQ(events[1].key.code,   KeyCode::A);
    EXPECT_EQ(events[2].key.code,   KeyCode::S);
    EXPECT_EQ(events[2].key.action, KeyAction::Released);
}

TEST_F(EvdevKeyboardTest, PollIgnoresNonKeyEvents)
{
    auto kb = make_keyboard();
    write_ev(sv[0], EV_SYN, 0,      0);   // sync - ignored
    write_ev(sv[0], EV_MSC, 4,     30);   // misc - ignored
    write_ev(sv[0], EV_KEY, KEY_D,  1);   // D pressed - delivered

    std::vector<InputEvent> events;
    ASSERT_TRUE(kb->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].key.code, KeyCode::D);
}

TEST_F(EvdevKeyboardTest, PollMapsUnknownKeyToUnknown)
{
    auto kb = make_keyboard();
    write_ev(sv[0], EV_KEY, 999, 1); // unmapped code

    std::vector<InputEvent> events;
    ASSERT_TRUE(kb->poll(events));
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].key.code, KeyCode::Unknown);
}

TEST_F(EvdevKeyboardTest, PollAppendsToExistingVector)
{
    auto kb = make_keyboard();
    std::vector<InputEvent> events;
    // Pre-populate with one event
    events.push_back(InputEvent::make_key(KeyCode::Q, KeyAction::Pressed));

    write_ev(sv[0], EV_KEY, KEY_W, 1);

    ASSERT_TRUE(kb->poll(events));
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].key.code, KeyCode::Q); // original preserved
    EXPECT_EQ(events[1].key.code, KeyCode::W); // new event appended
}

TEST_F(EvdevKeyboardTest, PollTwiceDeliversPendingThenEmpty)
{
    auto kb = make_keyboard();
    write_ev(sv[0], EV_KEY, KEY_E, 1);

    std::vector<InputEvent> first;
    ASSERT_TRUE(kb->poll(first));
    ASSERT_EQ(first.size(), 1u);
    EXPECT_EQ(first[0].key.code, KeyCode::E);

    std::vector<InputEvent> second;
    ASSERT_TRUE(kb->poll(second)); // no new events
    EXPECT_TRUE(second.empty());
}