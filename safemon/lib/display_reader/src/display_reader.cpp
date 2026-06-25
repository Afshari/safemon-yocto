#include "display_reader.h"
#include <ctime>
#include <cstdio>

DisplayStateReader::DisplayStateReader(std::unique_ptr<IRedisClient> redis_client)
    : redis_client_(std::move(redis_client))
    , last_change_time_(time(nullptr))
{
}

std::string DisplayStateReader::extract_can_id(const std::string& frame) const
{
    auto pos = frame.find('#');
    if (pos == std::string::npos) return "----";
    return "0x" + frame.substr(0, pos);
}

DashboardState DisplayStateReader::read(bool& redis_ok)
{
    DashboardState state;
    state.redis_ok = redis_ok;

    if (!redis_ok) {
        state.can_ok     = false;
        state.fault_code = "APP DOWN";
        return state;
    }

    // --- last frame ---
    std::string frame = redis_client_->get_latest_frame();
    if (!frame.empty()) {
        state.last_frame = frame;
        state.can_ok     = true;

        if (frame != last_seen_frame_) {
            last_seen_frame_  = frame;
            last_change_time_ = time(nullptr);
        }

        double elapsed = difftime(time(nullptr), last_change_time_);
        char detail1_buf[64];
        snprintf(detail1_buf, sizeof(detail1_buf),
                 "No new frame received %.1fs", elapsed);
        state.fault_detail1 = detail1_buf;
        state.fault_detail2 = "Last known ID " + extract_can_id(frame);
    } else {
        state.can_ok = false;
    }

    // --- frame count ---
    state.frame_count = redis_client_->get_frame_count();

    // --- fault status ---
    std::string fault = redis_client_->get_fault_status();
    if (!fault.empty())
        state.fault_code = fault;
    else
        state.fault_code = "APP DOWN";

    return state;
}