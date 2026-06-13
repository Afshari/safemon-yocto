#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include "config.h"

class ConfigTest : public ::testing::Test {
protected:
    std::string path = "test_safemon.conf";

    void write_config(const std::string& content) {
        std::ofstream f(path);
        f << content;
    }

    void TearDown() override {
        std::remove(path.c_str());
    }
};

TEST_F(ConfigTest, DefaultsWhenFileMissing) {
    SafemonConfig cfg = load_config("nonexistent_file.conf");

    EXPECT_EQ(cfg.drm_device, "/dev/dri/card1");
    EXPECT_EQ(cfg.redis_host, "127.0.0.1");
    EXPECT_EQ(cfg.redis_port, 6379);
    EXPECT_EQ(cfg.timeout_seconds, 5);
    EXPECT_EQ(cfg.known_ids.size(), 3u);
}

TEST_F(ConfigTest, ParsesAllFields) {
    write_config(
        "drm_device=/dev/dri/card0\n"
        "redis_host=192.168.1.10\n"
        "redis_port=6380\n"
        "known_ids=0x111,0x222,0x333\n"
        "timeout_seconds=10\n"
    );

    SafemonConfig cfg = load_config(path);

    EXPECT_EQ(cfg.drm_device, "/dev/dri/card0");
    EXPECT_EQ(cfg.redis_host, "192.168.1.10");
    EXPECT_EQ(cfg.redis_port, 6380);
    EXPECT_EQ(cfg.timeout_seconds, 10);
    EXPECT_EQ(cfg.known_ids.size(), 3u);
    EXPECT_TRUE(cfg.known_ids.count(0x111));
    EXPECT_TRUE(cfg.known_ids.count(0x222));
    EXPECT_TRUE(cfg.known_ids.count(0x333));
}

TEST_F(ConfigTest, IgnoresCommentsAndBlankLines) {
    write_config(
        "# this is a comment\n"
        "\n"
        "redis_port=7000\n"
        "   \n"
        "# another comment\n"
        "timeout_seconds=15\n"
    );

    SafemonConfig cfg = load_config(path);

    EXPECT_EQ(cfg.redis_port, 7000);
    EXPECT_EQ(cfg.timeout_seconds, 15);
}

TEST_F(ConfigTest, TrimsWhitespaceAroundKeyAndValue) {
    write_config(
        "  redis_host  =  10.0.0.5  \n"
        "redis_port=1234\n"
    );

    SafemonConfig cfg = load_config(path);

    EXPECT_EQ(cfg.redis_host, "10.0.0.5");
    EXPECT_EQ(cfg.redis_port, 1234);
}

TEST_F(ConfigTest, ParsesKnownIdsWithWhitespace) {
    write_config(
        "known_ids = 0x123, 0x456 , 0x789\n"
    );

    SafemonConfig cfg = load_config(path);

    EXPECT_EQ(cfg.known_ids.size(), 3u);
    EXPECT_TRUE(cfg.known_ids.count(0x123));
    EXPECT_TRUE(cfg.known_ids.count(0x456));
    EXPECT_TRUE(cfg.known_ids.count(0x789));
}

TEST_F(ConfigTest, SkipsInvalidKnownId) {
    write_config(
        "known_ids = 0x123, not_a_number, 0x789\n"
    );

    SafemonConfig cfg = load_config(path);

    EXPECT_EQ(cfg.known_ids.size(), 2u);
    EXPECT_TRUE(cfg.known_ids.count(0x123));
    EXPECT_TRUE(cfg.known_ids.count(0x789));
}

TEST_F(ConfigTest, IgnoresUnknownKey) {
    write_config(
        "redis_port=9999\n"
        "totally_unknown_key=foo\n"
    );

    SafemonConfig cfg = load_config(path);

    EXPECT_EQ(cfg.redis_port, 9999);
}