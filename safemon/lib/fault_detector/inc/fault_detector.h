#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <hiredis/hiredis.h>
#include "config.h"
#include "fault_rules.h"

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
    std::string get_latest_frame();
    void publish_fault(const std::string& level, const std::string& message);

    redisContext*     redis_;
    std::thread       thread_;
    std::atomic<bool> running_;
    std::chrono::steady_clock::time_point last_frame_time_;
    std::string       last_seen_frame_;
    FaultRules        rules_;
};