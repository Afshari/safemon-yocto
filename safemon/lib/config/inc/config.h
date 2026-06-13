#pragma once
#include <string>
#include <set>
#include <stdint.h>

struct SafemonConfig {
    std::string          drm_device      = "/dev/dri/card1";
    std::string          redis_host      = "127.0.0.1";
    int                  redis_port      = 6379;
    std::set<uint32_t>   known_ids       = { 0x123, 0x456, 0x789 };
    int                  timeout_seconds = 5;
};

SafemonConfig load_config(const std::string& path);