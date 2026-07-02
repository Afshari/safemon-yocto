#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "input_manager.h"

using ::testing::Return;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NiceMock;

// ---------------------------------------------------------------------------
// MockInputDevice
// ---------------------------------------------------------------------------

class MockInputDevice : public IInputDevice {
public:
    MOCK_METHOD(bool, open,    (),                           (override));
    MOCK_METHOD(void, close,   (),                           (override));
    MOCK_METHOD(bool, is_open, (),                           (const, override));
    MOCK_METHOD(bool, poll,    (std::vector<InputEvent>&),   (override));
};

// Helper: returns a poll() action that appends a key event to the out vector.
ACTION_P2(AppendKeyEvent, code, action) {
    arg0.push_back(InputEvent::make_key(code, action));
    return true;
}

// Helper: returns a poll() action that appends a mouse move event.
ACTION_P2(AppendMouseMove, dx, dy) {
    arg0.push_back(InputEvent::make_mouse_move(dx, dy));
    return true;
}

// ---------------------------------------------------------------------------
// Fixture helpers
// ---------------------------------------------------------------------------

static std::unique_ptr<MockInputDevice> make_mock()
{
    return std::make_unique<NiceMock<MockInputDevice>>();
}

// Build an InputManager from two raw mock pointers (ownership transferred).
// Keeps raw pointers for EXPECT_CALL setup.
struct ManagedMocks {
    MockInputDevice*             kb_raw;
    MockInputDevice*             mouse_raw;
    std::unique_ptr<InputManager> mgr;
};

static ManagedMocks make_managed()
{
    auto kb    = make_mock();
    auto mouse = make_mock();
    auto* kr   = kb.get();
    auto* mr   = mouse.get();
    return { kr, mr, std::make_unique<InputManager>(std::move(kb), std::move(mouse)) };
}

// ---------------------------------------------------------------------------
// open()
// ---------------------------------------------------------------------------

TEST(InputManagerOpen, ReturnsTrueWhenBothDevicesOpen)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(true));
    EXPECT_TRUE(mgr->open());
    EXPECT_TRUE(mgr->keyboard_available());
    EXPECT_TRUE(mgr->mouse_available());
}

TEST(InputManagerOpen, ReturnsTrueWhenOnlyKeyboardOpens)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(false));
    EXPECT_TRUE(mgr->open());
    EXPECT_TRUE(mgr->keyboard_available());
    EXPECT_FALSE(mgr->mouse_available());
}

TEST(InputManagerOpen, ReturnsTrueWhenOnlyMouseOpens)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(false));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(true));
    EXPECT_TRUE(mgr->open());
    EXPECT_FALSE(mgr->keyboard_available());
    EXPECT_TRUE(mgr->mouse_available());
}

TEST(InputManagerOpen, ReturnsFalseWhenNoDeviceOpens)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(false));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(false));
    EXPECT_FALSE(mgr->open());
    EXPECT_FALSE(mgr->keyboard_available());
    EXPECT_FALSE(mgr->mouse_available());
}

TEST(InputManagerOpen, NullKeyboardIsSkipped)
{
    auto mouse = make_mock();
    auto* mr   = mouse.get();
    InputManager mgr(nullptr, std::move(mouse));
    EXPECT_CALL(*mr, open()).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open());
    EXPECT_FALSE(mgr.keyboard_available()); // no keyboard
    EXPECT_TRUE(mgr.mouse_available());
}

TEST(InputManagerOpen, NullMouseIsSkipped)
{
    auto kb   = make_mock();
    auto* kr  = kb.get();
    InputManager mgr(std::move(kb), nullptr);
    EXPECT_CALL(*kr, open()).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open());
    EXPECT_TRUE(mgr.keyboard_available());
    EXPECT_FALSE(mgr.mouse_available()); // no mouse
}

TEST(InputManagerOpen, BothNullReturnsFalse)
{
    InputManager mgr(nullptr, nullptr);
    EXPECT_FALSE(mgr.open());
    EXPECT_FALSE(mgr.keyboard_available());
    EXPECT_FALSE(mgr.mouse_available());
}

// ---------------------------------------------------------------------------
// close()
// ---------------------------------------------------------------------------

TEST(InputManagerClose, ClosesBothDevices)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(true));
    mgr->open();

    // Expect at least one close() - destructor will also call close()
    // so the total count per device may be 2.
    EXPECT_CALL(*kb,    close()).Times(::testing::AtLeast(1));
    EXPECT_CALL(*mouse, close()).Times(::testing::AtLeast(1));
    mgr->close();

    EXPECT_FALSE(mgr->keyboard_available());
    EXPECT_FALSE(mgr->mouse_available());
}

TEST(InputManagerClose, SafeToCallWithNullDevices)
{
    InputManager mgr(nullptr, nullptr);
    EXPECT_NO_THROW(mgr.close());
}

// ---------------------------------------------------------------------------
// poll() - event delivery
// ---------------------------------------------------------------------------

TEST(InputManagerPoll, ReturnsZeroWhenNoDevicesAvailable)
{
    auto [kb, mouse, mgr] = make_managed();
    // Devices never opened - poll() should not call device poll()
    EXPECT_CALL(*kb,    poll(_)).Times(0);
    EXPECT_CALL(*mouse, poll(_)).Times(0);

    auto events = mgr->poll();
    EXPECT_TRUE(events.empty());
}

TEST(InputManagerPoll, PollsKeyboardAndMouseInOrder)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(true));
    mgr->open();

    EXPECT_CALL(*kb,    poll(_)).WillOnce(AppendKeyEvent(KeyCode::W, KeyAction::Pressed));
    EXPECT_CALL(*mouse, poll(_)).WillOnce(AppendMouseMove(5, -3));

    auto events = mgr->poll();
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].type,       InputEventType::Key);       // keyboard first
    EXPECT_EQ(events[0].key.code,   KeyCode::W);
    EXPECT_EQ(events[1].type,       InputEventType::MouseMove);  // mouse second
    EXPECT_EQ(events[1].mouse_move.dx, 5);
}

TEST(InputManagerPoll, AppendOverloadPreservesExistingEvents)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(false));
    mgr->open();

    EXPECT_CALL(*kb, poll(_)).WillOnce(AppendKeyEvent(KeyCode::S, KeyAction::Pressed));

    std::vector<InputEvent> events;
    events.push_back(InputEvent::make_key(KeyCode::Q, KeyAction::Released));

    size_t added = mgr->poll(events);
    EXPECT_EQ(added, 1u);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].key.code, KeyCode::Q); // original preserved
    EXPECT_EQ(events[1].key.code, KeyCode::S); // new event appended
}

TEST(InputManagerPoll, ReturnsCorrectCountOfAddedEvents)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(true));
    mgr->open();

    // Keyboard adds 2 events, mouse adds 1
    EXPECT_CALL(*kb, poll(_)).WillOnce(Invoke([](std::vector<InputEvent>& out) {
        out.push_back(InputEvent::make_key(KeyCode::W, KeyAction::Pressed));
        out.push_back(InputEvent::make_key(KeyCode::A, KeyAction::Pressed));
        return true;
    }));
    EXPECT_CALL(*mouse, poll(_)).WillOnce(AppendMouseMove(1, 2));

    std::vector<InputEvent> events;
    size_t added = mgr->poll(events);
    EXPECT_EQ(added, 3u);
    EXPECT_EQ(events.size(), 3u);
}

TEST(InputManagerPoll, SkipsKeyboardWhenOnlyMouseAvailable)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(false));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(true));
    mgr->open();

    EXPECT_CALL(*kb,    poll(_)).Times(0); // keyboard not available - never polled
    EXPECT_CALL(*mouse, poll(_)).WillOnce(AppendMouseMove(2, 3));

    auto events = mgr->poll();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, InputEventType::MouseMove);
}

TEST(InputManagerPoll, EmptyPollWhenBothDevicesReturnNoEvents)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(true));
    mgr->open();

    EXPECT_CALL(*kb,    poll(_)).WillOnce(Return(true));
    EXPECT_CALL(*mouse, poll(_)).WillOnce(Return(true));

    auto events = mgr->poll();
    EXPECT_TRUE(events.empty());
}

// ---------------------------------------------------------------------------
// Disconnect handling
// ---------------------------------------------------------------------------

TEST(InputManagerPoll, MarksKeyboardUnavailableOnDisconnect)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(false));
    mgr->open();
    EXPECT_TRUE(mgr->keyboard_available());

    // poll() returns false - device disconnected
    EXPECT_CALL(*kb, poll(_)).WillOnce(Return(false));
    mgr->poll();

    EXPECT_FALSE(mgr->keyboard_available());
}

TEST(InputManagerPoll, MarksMouseUnavailableOnDisconnect)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(false));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(true));
    mgr->open();

    EXPECT_CALL(*mouse, poll(_)).WillOnce(Return(false));
    mgr->poll();

    EXPECT_FALSE(mgr->mouse_available());
}

TEST(InputManagerPoll, DoesNotPollDisconnectedDeviceAgain)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(false));
    mgr->open();

    // First poll - disconnect
    EXPECT_CALL(*kb, poll(_)).WillOnce(Return(false));
    mgr->poll();

    // Second poll - keyboard should not be polled again
    EXPECT_CALL(*kb, poll(_)).Times(0);
    mgr->poll();
}

// ---------------------------------------------------------------------------
// reopen()
// ---------------------------------------------------------------------------

TEST(InputManagerReopen, ReconnectsDisconnectedKeyboard)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    // Mouse fails on initial open and on reopen attempt
    EXPECT_CALL(*mouse, open()).WillRepeatedly(Return(false));
    mgr->open();

    // Disconnect keyboard
    EXPECT_CALL(*kb, poll(_)).WillOnce(Return(false));
    mgr->poll();
    EXPECT_FALSE(mgr->keyboard_available());

    // Reopen - keyboard comes back
    EXPECT_CALL(*kb, open()).WillOnce(Return(true));
    bool ok = mgr->reopen();
    EXPECT_TRUE(ok);
    EXPECT_TRUE(mgr->keyboard_available());
}

TEST(InputManagerReopen, DoesNotReopenAlreadyOpenDevice)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(true));
    mgr->open();

    // reopen() should not call open() on devices that are already available
    EXPECT_CALL(*kb,    open()).Times(0);
    EXPECT_CALL(*mouse, open()).Times(0);
    mgr->reopen();
}

TEST(InputManagerReopen, ReturnsFalseWhenReopenFails)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(false));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(false));
    mgr->open();

    // Both still unavailable on reopen attempt
    EXPECT_CALL(*kb,    open()).WillOnce(Return(false));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(false));
    EXPECT_FALSE(mgr->reopen());
}

TEST(InputManagerReopen, ReturnsTrueWhenEitherDeviceReconnects)
{
    auto [kb, mouse, mgr] = make_managed();
    EXPECT_CALL(*kb,    open()).WillOnce(Return(false));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(false));
    mgr->open();

    EXPECT_CALL(*kb,    open()).WillOnce(Return(true));
    EXPECT_CALL(*mouse, open()).WillOnce(Return(false));
    EXPECT_TRUE(mgr->reopen());
    EXPECT_TRUE(mgr->keyboard_available());
    EXPECT_FALSE(mgr->mouse_available());
}