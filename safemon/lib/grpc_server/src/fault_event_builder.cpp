#include "fault_event_builder.h"
#include <chrono>

namespace safemon {

    bool build_fault_event(
        const std::string& current_status,
        const std::string& last_status,
        FaultEventData&    out_event)
    {
        if (current_status == last_status)
            return false;

        out_event.status       = current_status;
        out_event.source       = "safemon-app";
        out_event.timestamp_ms = now_ms();
        return true;
    }

    int64_t now_ms()
    {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    }

} // namespace safemon