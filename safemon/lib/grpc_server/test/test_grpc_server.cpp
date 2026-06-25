#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
#include "grpc_server.h"
#include "fault_event_builder.h"
#include "redis_client.h"
#include "config.h"

using ::testing::Return;
using ::testing::_;

// ---------------------------------------------------------------------------
// Mock
// ---------------------------------------------------------------------------

class MockRedisClient : public IRedisClient
{
public:
    MOCK_METHOD(std::string, get_latest_frame,  (), (override));
    MOCK_METHOD(std::string, get_fault_status,  (), (override));
    MOCK_METHOD(void,        publish_fault,
                (const std::string&, const std::string&), (override));
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SafemonConfig make_config()
{
    SafemonConfig cfg;
    cfg.redis_host      = "localhost";
    cfg.redis_port      = 6379;
    cfg.timeout_seconds = 5;
    return cfg;
}

// ---------------------------------------------------------------------------
// FaultEventBuilder tests -- pure logic, no gRPC
// ---------------------------------------------------------------------------

TEST(FaultEventBuilder, NoEventWhenStatusUnchanged)
{
    safemon::FaultEventData event;
    bool changed = safemon::build_fault_event("OK:NO_FAULT", "OK:NO_FAULT", event);
    EXPECT_FALSE(changed);
}

TEST(FaultEventBuilder, EventWhenStatusChanges)
{
    safemon::FaultEventData event;
    bool changed = safemon::build_fault_event("ERROR:TIMEOUT", "OK:NO_FAULT", event);
    EXPECT_TRUE(changed);
    EXPECT_EQ(event.status, "ERROR:TIMEOUT");
    EXPECT_EQ(event.source, "safemon-app");
    EXPECT_GT(event.timestamp_ms, 0);
}

TEST(FaultEventBuilder, EventWhenLastStatusEmpty)
{
    safemon::FaultEventData event;
    bool changed = safemon::build_fault_event("OK:NO_FAULT", "", event);
    EXPECT_TRUE(changed);
    EXPECT_EQ(event.status, "OK:NO_FAULT");
}

TEST(FaultEventBuilder, NoEventWhenBothEmpty)
{
    safemon::FaultEventData event;
    bool changed = safemon::build_fault_event("", "", event);
    EXPECT_FALSE(changed);
}

TEST(FaultEventBuilder, EventWhenStatusChangesToWarn)
{
    safemon::FaultEventData event;
    bool changed = safemon::build_fault_event(
        "WARN:UNKNOWN_ID:0x999", "OK:NO_FAULT", event);
    EXPECT_TRUE(changed);
    EXPECT_EQ(event.status, "WARN:UNKNOWN_ID:0x999");
    EXPECT_EQ(event.source, "safemon-app");
}

TEST(FaultEventBuilder, TimestampIsReasonable)
{
    safemon::FaultEventData event;
    safemon::build_fault_event("ERROR:TIMEOUT", "OK:NO_FAULT", event);

    // timestamp should be within the last minute
    int64_t now = safemon::now_ms();
    EXPECT_GT(event.timestamp_ms, now - 60000);
    EXPECT_LE(event.timestamp_ms, now);
}

// ---------------------------------------------------------------------------
// GrpcServer lifecycle tests
// ---------------------------------------------------------------------------

TEST(GrpcServer, StartsAndStopsCleanly)
{
    auto mock = std::make_unique<MockRedisClient>();
    EXPECT_CALL(*mock, get_fault_status())
        .WillRepeatedly(Return("OK:NO_FAULT"));
    EXPECT_CALL(*mock, get_latest_frame())
        .WillRepeatedly(Return(""));
    EXPECT_CALL(*mock, publish_fault(_, _))
        .WillRepeatedly(Return());

    GrpcServer server(make_config(), "0.0.0.0:50099", std::move(mock));
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server.stop();
    // if we reach here without deadlock or crash the test passes
}

TEST(GrpcServer, StopsCleanlyWithoutStart)
{
    auto mock = std::make_unique<MockRedisClient>();
    EXPECT_CALL(*mock, get_fault_status())
        .WillRepeatedly(Return("OK:NO_FAULT"));
    EXPECT_CALL(*mock, get_latest_frame())
        .WillRepeatedly(Return(""));
    EXPECT_CALL(*mock, publish_fault(_, _))
        .WillRepeatedly(Return());

    GrpcServer server(make_config(), "0.0.0.0:50098", std::move(mock));
    // stop without start should not crash or deadlock
    server.stop();
}