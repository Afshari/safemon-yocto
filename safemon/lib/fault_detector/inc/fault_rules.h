#pragma once
#include <string>
#include <chrono>
#include "config.h"

class FaultRules {
public:
    explicit FaultRules(const SafemonConfig& cfg) : cfg_(cfg) {}

    bool check_timeout(std::chrono::steady_clock::time_point last_frame_time,
                        std::chrono::steady_clock::time_point now) const;

    bool check_unknown_id(uint32_t id) const;

    uint32_t parse_id(const std::string& frame) const;

    // Returns {level, message}, e.g. {"OK", "NO_FAULT"} or
    // {"ERROR", "TIMEOUT:no new frame"}
    std::pair<std::string, std::string> evaluate(
        const std::string& frame,
        bool frame_changed,
        std::chrono::steady_clock::time_point last_frame_time,
        std::chrono::steady_clock::time_point now) const;

private:
    SafemonConfig cfg_;
};