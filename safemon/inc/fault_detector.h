#pragma once
#include <string>
#include <set>
#include <thread>
#include <atomic>
#include <chrono>
#include <hiredis/hiredis.h>
#include "config.h"

const int DETECTOR_INTERVAL = 1;

class FaultDetector {
public:
    FaultDetector(const SafemonConfig& cfg);
    void start();
    void stop();

    ~FaultDetector() {
        if (redis_) redisFree(redis_);
    }

private:
    void run();

    bool        check_timeout();
    bool        check_unknown_id(uint32_t id);
    void        publish_fault(const std::string& level,
                              const std::string& message);
    std::string get_latest_frame();
    uint32_t    parse_id(const std::string& frame);

    redisContext*     redis_;
    std::thread       thread_;
    std::atomic<bool> running_;
    std::chrono::steady_clock::time_point last_frame_time_;
    std::string       last_seen_frame_;
    SafemonConfig cfg_;
};