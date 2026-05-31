#pragma once
#include <string>
#include <set>
#include <thread>
#include <atomic>
#include <chrono>
#include <hiredis/hiredis.h>

// Known valid CAN IDs
const std::set<uint32_t> KNOWN_IDS = { 0x123, 0x456, 0x789 };

// Fault thresholds
const int TIMEOUT_SECONDS   = 5;
const int DETECTOR_INTERVAL = 1; // check every 1 second

class FaultDetector {
public:
    FaultDetector(redisContext* redis);
    void start();
    void stop();

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
};