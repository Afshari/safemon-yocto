#include "fault_detector.h"
#include <iostream>
#include <sstream>
#include <unistd.h>

FaultDetector::FaultDetector(redisContext* redis)
    : redis_(redis)
    , running_(false)
    , last_frame_time_(std::chrono::steady_clock::now())
    , last_seen_frame_("") {}

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

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void FaultDetector::run() {
    while (running_) {
        std::string frame = get_latest_frame();

        if (frame.empty()) {
            // no frames in Redis at all
            if (check_timeout())
                publish_fault("ERROR", "TIMEOUT:no frame received");
            else
                publish_fault("OK", "NO_FAULT");
        } else {
            // update last seen time if frame changed
            if (frame != last_seen_frame_) {
                last_seen_frame_  = frame;
                last_frame_time_  = std::chrono::steady_clock::now();
            }

            // check timeout
            if (check_timeout()) {
                publish_fault("ERROR", "TIMEOUT:no new frame");
            } else {
                // check unknown ID
                uint32_t id = parse_id(frame);
                if (check_unknown_id(id)) {
                    std::ostringstream oss;
                    oss << "UNKNOWN_ID:0x" << std::hex << id;
                    publish_fault("WARN", oss.str());
                } else {
                    publish_fault("OK", "NO_FAULT");
                }
            }
        }

        sleep(DETECTOR_INTERVAL);
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
std::string FaultDetector::get_latest_frame() {
    redisReply* reply = (redisReply*)redisCommand(
        redis_, "LINDEX safemon:can:frames 0");
    std::string result;
    if (reply && reply->type == REDIS_REPLY_STRING)
        result = std::string(reply->str, reply->len);
    if (reply) freeReplyObject(reply);
    return result;
}

uint32_t FaultDetector::parse_id(const std::string& frame) {
    // frame format: "123#DEADBEEF"
    uint32_t id = 0;
    std::istringstream iss(frame);
    std::string id_str;
    std::getline(iss, id_str, '#');
    try { id = std::stoul(id_str, nullptr, 16); }
    catch (...) { id = 0xFFFFFFFF; }
    return id;
}

bool FaultDetector::check_timeout() {
    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                   now - last_frame_time_).count();
    return elapsed >= TIMEOUT_SECONDS;
}

bool FaultDetector::check_unknown_id(uint32_t id) {
    return KNOWN_IDS.find(id) == KNOWN_IDS.end();
}

void FaultDetector::publish_fault(const std::string& level,
                                   const std::string& message) {
    std::string fault = level + ":" + message;
    redisReply* reply = (redisReply*)redisCommand(
        redis_, "SET safemon:faults:current %s", fault.c_str());
    if (reply) freeReplyObject(reply);
}