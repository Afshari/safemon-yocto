#include <gtest/gtest.h>
#include "fault_rules.h"

using namespace std::chrono;

static SafemonConfig make_config() {
    SafemonConfig cfg;
    cfg.known_ids = {0x123, 0x456, 0x789};
    cfg.timeout_seconds = 5;
    return cfg;
}

TEST(FaultRules, CheckTimeoutBelowThreshold) {
    FaultRules rules(make_config());
    auto now = steady_clock::now();
    auto last = now - seconds(3);

    EXPECT_FALSE(rules.check_timeout(last, now));
}

TEST(FaultRules, CheckTimeoutAtThreshold) {
    FaultRules rules(make_config());
    auto now = steady_clock::now();
    auto last = now - seconds(5);

    EXPECT_TRUE(rules.check_timeout(last, now));
}

TEST(FaultRules, CheckTimeoutAboveThreshold) {
    FaultRules rules(make_config());
    auto now = steady_clock::now();
    auto last = now - seconds(10);

    EXPECT_TRUE(rules.check_timeout(last, now));
}

TEST(FaultRules, KnownIdIsNotUnknown) {
    FaultRules rules(make_config());
    EXPECT_FALSE(rules.check_unknown_id(0x123));
    EXPECT_FALSE(rules.check_unknown_id(0x456));
    EXPECT_FALSE(rules.check_unknown_id(0x789));
}

TEST(FaultRules, UnknownIdIsDetected) {
    FaultRules rules(make_config());
    EXPECT_TRUE(rules.check_unknown_id(0x999));
}

TEST(FaultRules, ParseIdValidFrame) {
    FaultRules rules(make_config());
    EXPECT_EQ(rules.parse_id("123#DEADBEEF"), 0x123u);
    EXPECT_EQ(rules.parse_id("456#00"), 0x456u);
}

TEST(FaultRules, ParseIdInvalidFrame) {
    FaultRules rules(make_config());
    EXPECT_EQ(rules.parse_id("not_a_valid_frame"), 0xFFFFFFFFu);
    EXPECT_EQ(rules.parse_id(""), 0xFFFFFFFFu);
}

TEST(FaultRules, EvaluateEmptyFrameNoTimeout) {
    FaultRules rules(make_config());
    auto now  = steady_clock::now();
    auto last = now - seconds(1);

    auto [level, msg] = rules.evaluate("", false, last, now);
    EXPECT_EQ(level, "OK");
    EXPECT_EQ(msg, "NO_FAULT");
}

TEST(FaultRules, EvaluateEmptyFrameWithTimeout) {
    FaultRules rules(make_config());
    auto now  = steady_clock::now();
    auto last = now - seconds(10);

    auto [level, msg] = rules.evaluate("", false, last, now);
    EXPECT_EQ(level, "ERROR");
    EXPECT_EQ(msg, "TIMEOUT:no frame received");
}

TEST(FaultRules, EvaluateKnownIdFrameOk) {
    FaultRules rules(make_config());
    auto now  = steady_clock::now();
    auto last = now - seconds(1);

    auto [level, msg] = rules.evaluate("123#DEADBEEF", true, last, now);
    EXPECT_EQ(level, "OK");
    EXPECT_EQ(msg, "NO_FAULT");
}

TEST(FaultRules, EvaluateUnknownIdFrameWarn) {
    FaultRules rules(make_config());
    auto now  = steady_clock::now();
    auto last = now - seconds(1);

    auto [level, msg] = rules.evaluate("999#CAFEBABE", true, last, now);
    EXPECT_EQ(level, "WARN");
    EXPECT_EQ(msg, "UNKNOWN_ID:0x999");
}

TEST(FaultRules, EvaluateTimeoutOverridesUnknownId) {
    FaultRules rules(make_config());
    auto now  = steady_clock::now();
    auto last = now - seconds(10);

    auto [level, msg] = rules.evaluate("999#CAFEBABE", false, last, now);
    EXPECT_EQ(level, "ERROR");
    EXPECT_EQ(msg, "TIMEOUT:no new frame");
}