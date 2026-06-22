#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include "config.h"
#include "fault_rules.h"
#include "redis_client.h"

class FaultDetector
{
public:
    FaultDetector(const SafemonConfig& cfg,
                  std::unique_ptr<IRedisClient> redis_client,
                  int interval_seconds = 1);

    void start();
    void stop();

    ~FaultDetector() { stop(); }

    // non-copyable
    FaultDetector(const FaultDetector&)            = delete;
    FaultDetector& operator=(const FaultDetector&) = delete;

private:
    void run();

    std::unique_ptr<IRedisClient>          redis_client_;
    std::thread                            thread_;
    std::atomic<bool>                      running_;
    std::chrono::steady_clock::time_point  last_frame_time_;
    std::string                            last_seen_frame_;
    FaultRules                             rules_;
    int                                    interval_seconds_;
};