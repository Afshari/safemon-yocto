#include "fault_detector.h"
#include <iostream>
#include <unistd.h>

FaultDetector::FaultDetector(const SafemonConfig& cfg,
                              std::unique_ptr<IRedisClient> redis_client,
                              int interval_seconds)
    : redis_client_(std::move(redis_client))
    , running_(false)
    , last_frame_time_(std::chrono::steady_clock::now())
    , last_seen_frame_("")
    , rules_(cfg)
    , interval_seconds_(interval_seconds)
{
}

void FaultDetector::start()
{
    running_ = true;
    thread_  = std::thread(&FaultDetector::run, this);
    std::cout << "[fault] Detector started\n";
}

void FaultDetector::stop()
{
    running_ = false;
    if (thread_.joinable())
        thread_.join();
    std::cout << "[fault] Detector stopped\n";
}

void FaultDetector::run()
{
    while (running_) {
        std::string frame = redis_client_->get_latest_frame();
        auto now = std::chrono::steady_clock::now();

        bool frame_changed = (!frame.empty() && frame != last_seen_frame_);
        if (frame_changed) {
            last_seen_frame_ = frame;
            last_frame_time_ = now;
        }

        auto [level, message] = rules_.evaluate(
            frame, frame_changed, last_frame_time_, now);

        redis_client_->publish_fault(level, message);

        sleep(interval_seconds_);
    }
}