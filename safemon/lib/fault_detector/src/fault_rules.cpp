#include "fault_rules.h"
#include <sstream>

bool FaultRules::check_timeout(
    std::chrono::steady_clock::time_point last_frame_time,
    std::chrono::steady_clock::time_point now) const
{
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                   now - last_frame_time).count();
    return elapsed >= cfg_.timeout_seconds;
}

bool FaultRules::check_unknown_id(uint32_t id) const
{
    return cfg_.known_ids.find(id) == cfg_.known_ids.end();
}

uint32_t FaultRules::parse_id(const std::string& frame) const
{
    uint32_t id = 0;
    std::istringstream iss(frame);
    std::string id_str;
    std::getline(iss, id_str, '#');
    try { id = std::stoul(id_str, nullptr, 16); }
    catch (...) { id = 0xFFFFFFFF; }
    return id;
}

std::pair<std::string, std::string> FaultRules::evaluate(
    const std::string& frame,
    bool frame_changed,
    std::chrono::steady_clock::time_point last_frame_time,
    std::chrono::steady_clock::time_point now) const
{
    if (frame.empty()) {
        if (check_timeout(last_frame_time, now))
            return {"ERROR", "TIMEOUT:no frame received"};
        return {"OK", "NO_FAULT"};
    }

    if (check_timeout(last_frame_time, now))
        return {"ERROR", "TIMEOUT:no new frame"};

    uint32_t id = parse_id(frame);
    if (check_unknown_id(id)) {
        std::ostringstream oss;
        oss << "UNKNOWN_ID:0x" << std::hex << id;
        return {"WARN", oss.str()};
    }

    return {"OK", "NO_FAULT"};
}