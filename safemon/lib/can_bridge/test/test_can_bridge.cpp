#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
#include "can_bridge.h"
#include "ican_reader.h"
#include "frame_store.h"

using ::testing::Return;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::InSequence;

// ---------------------------------------------------------------------------
// Mocks
// ---------------------------------------------------------------------------

class MockCanReader : public ICanReader
{
public:
    MOCK_METHOD(bool, open,  (), (override));
    MOCK_METHOD(bool, read,  (CanFrame& frame), (override));
    MOCK_METHOD(void, close, (), (override));
};

class MockFrameStore : public IFrameStore
{
public:
    MOCK_METHOD(void, push_frame, (const std::string& frame), (override));
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static CanFrame make_frame(uint32_t id, uint8_t len,
                           std::initializer_list<uint8_t> data)
{
    CanFrame f;
    f.id  = id;
    f.len = len;
    int i = 0;
    for (auto b : data) f.data[i++] = b;
    return f;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST(CanBridge, DoesNotStartIfCanOpenFails)
{
    auto can   = std::make_unique<MockCanReader>();
    auto store = std::make_unique<MockFrameStore>();

    EXPECT_CALL(*can, open()).WillOnce(Return(false));
    EXPECT_CALL(*can, close()).Times(0);
    EXPECT_CALL(*store, push_frame(_)).Times(0);

    CanBridge bridge(std::move(can), std::move(store));
    bridge.start();
    // no stop needed - thread never started
}

TEST(CanBridge, PushesFrameStringOnRead)
{
    auto can   = std::make_unique<MockCanReader>();
    auto store = std::make_unique<MockFrameStore>();

    CanFrame frame = make_frame(0x123, 4, {0xDE, 0xAD, 0xBE, 0xEF});

    EXPECT_CALL(*can, open()).WillOnce(Return(true));
    EXPECT_CALL(*can, close()).Times(1);
    EXPECT_CALL(*can, read(_))
        .WillOnce([frame](CanFrame& f) { f = frame; return true; })
        .WillRepeatedly(Return(false)); // timeout after first frame

    EXPECT_CALL(*store, push_frame("123#DEADBEEF"))
        .Times(AtLeast(1));

    CanBridge bridge(std::move(can), std::move(store));
    bridge.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    bridge.stop();
}

TEST(CanBridge, SkipsReadWhenReturnsFalse)
{
    auto can   = std::make_unique<MockCanReader>();
    auto store = std::make_unique<MockFrameStore>();

    EXPECT_CALL(*can, open()).WillOnce(Return(true));
    EXPECT_CALL(*can, close()).Times(1);
    EXPECT_CALL(*can, read(_)).WillRepeatedly(Return(false));

    // no frames read - push_frame should never be called
    EXPECT_CALL(*store, push_frame(_)).Times(0);

    CanBridge bridge(std::move(can), std::move(store));
    bridge.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    bridge.stop();
}

TEST(CanBridge, PushesMultipleFramesInOrder)
{
    auto can   = std::make_unique<MockCanReader>();
    auto store = std::make_unique<MockFrameStore>();

    CanFrame f1 = make_frame(0x123, 2, {0xAA, 0xBB});
    CanFrame f2 = make_frame(0x456, 3, {0x11, 0x22, 0x33});

    EXPECT_CALL(*can, open()).WillOnce(Return(true));
    EXPECT_CALL(*can, close()).Times(1);
    {
        InSequence seq;
        EXPECT_CALL(*can, read(_))
            .WillOnce([f1](CanFrame& f) { f = f1; return true; });
        EXPECT_CALL(*can, read(_))
            .WillOnce([f2](CanFrame& f) { f = f2; return true; });
        EXPECT_CALL(*can, read(_))
            .WillRepeatedly(Return(false));
    }

    {
        InSequence seq;
        EXPECT_CALL(*store, push_frame("123#AABB")).Times(1);
        EXPECT_CALL(*store, push_frame("456#112233")).Times(1);
    }

    CanBridge bridge(std::move(can), std::move(store));
    bridge.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    bridge.stop();
}

TEST(CanBridge, StopsCleanlyWithoutDeadlock)
{
    auto can   = std::make_unique<MockCanReader>();
    auto store = std::make_unique<MockFrameStore>();

    EXPECT_CALL(*can, open()).WillOnce(Return(true));
    EXPECT_CALL(*can, close()).Times(1);
    EXPECT_CALL(*can, read(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*store, push_frame(_)).WillRepeatedly(Return());

    CanBridge bridge(std::move(can), std::move(store));
    bridge.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    bridge.stop();
}

TEST(CanBridge, BuildsCorrectFrameStringForSingleByte)
{
    auto can   = std::make_unique<MockCanReader>();
    auto store = std::make_unique<MockFrameStore>();

    CanFrame frame = make_frame(0x789, 1, {0x0F});

    EXPECT_CALL(*can, open()).WillOnce(Return(true));
    EXPECT_CALL(*can, close()).Times(1);
    EXPECT_CALL(*can, read(_))
        .WillOnce([frame](CanFrame& f) { f = frame; return true; })
        .WillRepeatedly(Return(false));

    EXPECT_CALL(*store, push_frame("789#0F")).Times(AtLeast(1));

    CanBridge bridge(std::move(can), std::move(store));
    bridge.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    bridge.stop();
}