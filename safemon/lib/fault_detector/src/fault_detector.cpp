#include "fault_detector.h"
#include <iostream>
#include <unistd.h>

FaultDetector::FaultDetector(const SafemonConfig& cfg)
    : running_(false)
    , last_frame_time_(std::chrono::steady_clock::now())
    , last_seen_frame_("")
    , rules_(cfg)
{
    redis_ = redisConnect(cfg.redis_host.c_str(), cfg.redis_port);
    if (!redis_ || redis_->err)
        std::cerr << "[fault] Redis connection failed\n";
}

void FaultDetector::start() {
    running_ = true;
    thread_  = std::thread(&FaultDetector::run, this);
    std::cout << "[fault] Detector started\n";
}

void FaultDetector::stop() {
    running_ = false;
    if (thread_.joinable())
        thread_.join();
    std::cout << "[fault] Detector stopped\n";
}

void FaultDetector::run() {
    while (running_) {
        std::string frame = get_latest_frame();
        auto now = std::chrono::steady_clock::now();

        bool frame_changed = (!frame.empty() && frame != last_seen_frame_);
        if (frame_changed) {
            last_seen_frame_ = frame;
            last_frame_time_ = now;
        }

        auto [level, message] = rules_.evaluate(
            frame, frame_changed, last_frame_time_, now);
        publish_fault(level, message);

        sleep(DETECTOR_INTERVAL);
    }
}

std::string FaultDetector::get_latest_frame() {
    redisReply* reply = (redisReply*)redisCommand(
        redis_, "LINDEX safemon:can:frames 0");
    std::string result;
    if (reply && reply->type == REDIS_REPLY_STRING)
        result = std::string(reply->str, reply->len);
    if (reply) freeReplyObject(reply);
    return result;
}

void FaultDetector::publish_fault(const std::string& level,
                                   const std::string& message) {
    std::string fault = level + ":" + message;
    redisReply* reply = (redisReply*)redisCommand(
        redis_, "SET safemon:faults:current %s", fault.c_str());
    if (reply) freeReplyObject(reply);
}