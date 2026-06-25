#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>
#include "fault_detector.h"
#include "redis_client.h"

using ::testing::Return;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::InSequence;

// ---------------------------------------------------------------------------
// Mock
// ---------------------------------------------------------------------------

class MockRedisClient : public IRedisClient
{
public:
    MOCK_METHOD(std::string, get_latest_frame, (), (override));
    MOCK_METHOD(void, publish_fault,
                (const std::string& level, const std::string& message),
                (override));
    MOCK_METHOD(std::string, get_fault_status, (), (override));
    MOCK_METHOD(long, get_frame_count, (), (override));
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SafemonConfig make_config()
{
    SafemonConfig cfg;
    cfg.known_ids       = {0x123, 0x456, 0x789};
    cfg.timeout_seconds = 5;
    cfg.redis_host      = "localhost";
    cfg.redis_port      = 6379;
    return cfg;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST(FaultDetector, PublishesOkWhenKnownFrameArrives)
{
    auto mock = std::make_unique<MockRedisClient>();

    // Return a known frame, then keep returning it (no change after first)
    EXPECT_CALL(*mock, get_latest_frame())
        .WillOnce(Return("123#DEADBEEF"))
        .WillRepeatedly(Return("123#DEADBEEF"));

    EXPECT_CALL(*mock, publish_fault("OK", "NO_FAULT"))
        .Times(AtLeast(1));

    FaultDetector detector(make_config(), std::move(mock), 0);
    detector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    detector.stop();
}

TEST(FaultDetector, PublishesWarnOnUnknownId)
{
    auto mock = std::make_unique<MockRedisClient>();

    EXPECT_CALL(*mock, get_latest_frame())
        .WillRepeatedly(Return("999#CAFEBABE"));

    EXPECT_CALL(*mock, publish_fault("WARN", "UNKNOWN_ID:0x999"))
        .Times(AtLeast(1));

    FaultDetector detector(make_config(), std::move(mock), 0);
    detector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    detector.stop();
}

TEST(FaultDetector, PublishesErrorOnTimeout)
{
    auto mock = std::make_unique<MockRedisClient>();

    // Always return empty - simulates no frames on the bus
    EXPECT_CALL(*mock, get_latest_frame())
        .WillRepeatedly(Return(""));

    // With interval=0, the loop runs fast, but last_frame_time_ is set
    // to now() at construction - so we need to wait for timeout_seconds(5)
    // to elapse. Instead we set a very short timeout in config.
    SafemonConfig cfg     = make_config();
    cfg.timeout_seconds   = 0; // immediate timeout for test speed

    EXPECT_CALL(*mock, publish_fault("ERROR", _))
        .Times(AtLeast(1));

    FaultDetector detector(cfg, std::move(mock), 0);
    detector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    detector.stop();
}

TEST(FaultDetector, StopsCleanlyWithoutDeadlock)
{
    auto mock = std::make_unique<MockRedisClient>();

    EXPECT_CALL(*mock, get_latest_frame())
        .WillRepeatedly(Return("123#DEADBEEF"));
    EXPECT_CALL(*mock, publish_fault(_, _))
        .WillRepeatedly(Return());

    FaultDetector detector(make_config(), std::move(mock), 0);
    detector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // stop() must return within a reasonable time - if it deadlocks
    // the test runner will timeout and fail
    detector.stop();
}

TEST(FaultDetector, HandlesEmptyFrameGracefully)
{
    auto mock = std::make_unique<MockRedisClient>();

    EXPECT_CALL(*mock, get_latest_frame())
        .WillRepeatedly(Return(""));

    // Should not crash - just publish OK or ERROR depending on timeout
    EXPECT_CALL(*mock, publish_fault(_, _))
        .Times(AtLeast(1));

    FaultDetector detector(make_config(), std::move(mock), 0);
    detector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    detector.stop();
}

TEST(FaultDetector, ErrorTimeoutOverridesUnknownId)
{
    auto mock = std::make_unique<MockRedisClient>();

    // Unknown ID frame but no new frames arriving (frame_changed=false after first)
    EXPECT_CALL(*mock, get_latest_frame())
        .WillRepeatedly(Return("999#CAFEBABE"));

    SafemonConfig cfg   = make_config();
    cfg.timeout_seconds = 0; // immediate timeout

    // With timeout=0, ERROR should take priority over WARN
    EXPECT_CALL(*mock, publish_fault("ERROR", _))
        .Times(AtLeast(1));

    FaultDetector detector(cfg, std::move(mock), 0);
    detector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    detector.stop();
}

TEST(FaultDetector, PublishFaultIncludesLevelAndMessage)
{
    auto mock = std::make_unique<MockRedisClient>();

    EXPECT_CALL(*mock, get_latest_frame())
        .WillRepeatedly(Return("123#DEADBEEF"));

    // verify the exact string format passed to publish_fault
    EXPECT_CALL(*mock, publish_fault("OK", "NO_FAULT"))
        .Times(AtLeast(1));

    FaultDetector detector(make_config(), std::move(mock), 0);
    detector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    detector.stop();
}

TEST(FaultDetector, PublishFaultCalledOnEveryInterval)
{
    auto mock = std::make_unique<MockRedisClient>();

    EXPECT_CALL(*mock, get_latest_frame())
        .WillRepeatedly(Return("123#DEADBEEF"));

    // with interval=0, publish_fault should be called many times
    // this verifies the TTL refresh keeps happening while app runs
    EXPECT_CALL(*mock, publish_fault(_, _))
        .Times(AtLeast(5));

    FaultDetector detector(make_config(), std::move(mock), 0);
    detector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    detector.stop();
}