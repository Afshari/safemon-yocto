#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "display_reader.h"
#include "redis_client.h"

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
    MOCK_METHOD(long,        get_frame_count,   (), (override));
    MOCK_METHOD(void,        publish_fault,
                (const std::string&, const std::string&), (override));
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST(DisplayStateReader, ReturnsAppDownWhenRedisNotOk)
{
    auto mock = std::make_unique<MockRedisClient>();
    // no Redis calls expected when redis_ok is false
    EXPECT_CALL(*mock, get_latest_frame()).Times(0);
    EXPECT_CALL(*mock, get_fault_status()).Times(0);
    EXPECT_CALL(*mock, get_frame_count()).Times(0);

    DisplayStateReader reader(std::move(mock));
    bool redis_ok = false;
    DashboardState state = reader.read(redis_ok);

    EXPECT_FALSE(state.redis_ok);
    EXPECT_FALSE(state.can_ok);
    EXPECT_EQ(state.fault_code, "APP DOWN");
}

TEST(DisplayStateReader, SetsCanOkWhenFramePresent)
{
    auto mock = std::make_unique<MockRedisClient>();
    EXPECT_CALL(*mock, get_latest_frame()).WillOnce(Return("123#DEADBEEF"));
    EXPECT_CALL(*mock, get_fault_status()).WillOnce(Return("OK:NO_FAULT"));
    EXPECT_CALL(*mock, get_frame_count()).WillOnce(Return(42));

    DisplayStateReader reader(std::move(mock));
    bool redis_ok = true;
    DashboardState state = reader.read(redis_ok);

    EXPECT_TRUE(state.can_ok);
    EXPECT_TRUE(state.redis_ok);
    EXPECT_EQ(state.last_frame, "123#DEADBEEF");
    EXPECT_EQ(state.fault_code, "OK:NO_FAULT");
    EXPECT_EQ(state.frame_count, 42);
}

TEST(DisplayStateReader, SetsCanErrorWhenNoFrame)
{
    auto mock = std::make_unique<MockRedisClient>();
    EXPECT_CALL(*mock, get_latest_frame()).WillOnce(Return(""));
    EXPECT_CALL(*mock, get_fault_status()).WillOnce(Return("ERROR:TIMEOUT"));
    EXPECT_CALL(*mock, get_frame_count()).WillOnce(Return(0));

    DisplayStateReader reader(std::move(mock));
    bool redis_ok = true;
    DashboardState state = reader.read(redis_ok);

    EXPECT_FALSE(state.can_ok);
    EXPECT_EQ(state.fault_code, "ERROR:TIMEOUT");
    EXPECT_EQ(state.frame_count, 0);
}

TEST(DisplayStateReader, ExtractsCanIdFromFrame)
{
    auto mock = std::make_unique<MockRedisClient>();
    EXPECT_CALL(*mock, get_latest_frame()).WillOnce(Return("456#0102030405060708"));
    EXPECT_CALL(*mock, get_fault_status()).WillOnce(Return("OK:NO_FAULT"));
    EXPECT_CALL(*mock, get_frame_count()).WillOnce(Return(1));

    DisplayStateReader reader(std::move(mock));
    bool redis_ok = true;
    DashboardState state = reader.read(redis_ok);

    EXPECT_EQ(state.fault_detail2, "Last known ID 0x456");
}

TEST(DisplayStateReader, FaultCodeAppDownWhenFaultStatusEmpty)
{
    auto mock = std::make_unique<MockRedisClient>();
    EXPECT_CALL(*mock, get_latest_frame()).WillOnce(Return("123#DEADBEEF"));
    EXPECT_CALL(*mock, get_fault_status()).WillOnce(Return(""));
    EXPECT_CALL(*mock, get_frame_count()).WillOnce(Return(5));

    DisplayStateReader reader(std::move(mock));
    bool redis_ok = true;
    DashboardState state = reader.read(redis_ok);

    EXPECT_EQ(state.fault_code, "APP DOWN");
}

TEST(DisplayStateReader, ElapsedTimeIncreasesWhenFrameUnchanged)
{
    auto mock = std::make_unique<MockRedisClient>();

    // same frame both calls - elapsed time should increase
    EXPECT_CALL(*mock, get_latest_frame())
        .WillRepeatedly(Return("123#DEADBEEF"));
    EXPECT_CALL(*mock, get_fault_status())
        .WillRepeatedly(Return("OK:NO_FAULT"));
    EXPECT_CALL(*mock, get_frame_count())
        .WillRepeatedly(Return(1));

    DisplayStateReader reader(std::move(mock));
    bool redis_ok = true;

    DashboardState state1 = reader.read(redis_ok);
    // detail1 should contain "0.0s" or similar on first read
    EXPECT_NE(state1.fault_detail1.find("No new frame received"), std::string::npos);

    DashboardState state2 = reader.read(redis_ok);
    EXPECT_NE(state2.fault_detail1.find("No new frame received"), std::string::npos);
}

TEST(DisplayStateReader, ResetsElapsedWhenNewFrameArrives)
{
    auto mock = std::make_unique<MockRedisClient>();

    EXPECT_CALL(*mock, get_latest_frame())
        .WillOnce(Return("123#DEADBEEF"))
        .WillOnce(Return("456#AABBCCDD")); // new frame
    EXPECT_CALL(*mock, get_fault_status())
        .WillRepeatedly(Return("OK:NO_FAULT"));
    EXPECT_CALL(*mock, get_frame_count())
        .WillRepeatedly(Return(2));

    DisplayStateReader reader(std::move(mock));
    bool redis_ok = true;

    reader.read(redis_ok);          // first read - sets last_seen_frame_
    DashboardState state = reader.read(redis_ok); // new frame arrives

    // elapsed should be near zero after frame change
    EXPECT_EQ(state.last_frame, "456#AABBCCDD");
    EXPECT_EQ(state.fault_detail2, "Last known ID 0x456");
}