#pragma once
#include <string>
#include <cstdint>

namespace safemon {

    // Represents a fault event ready to be streamed to gRPC clients.
    struct FaultEventData
    {
        std::string status;
        std::string source;
        int64_t     timestamp_ms = 0;
    };


    // Determines whether a status change occurred and builds the event.
    // Returns true if status changed and event was populated.
    bool build_fault_event(
        const std::string& current_status,
        const std::string& last_status,
        FaultEventData&    out_event);

    // Returns current time in milliseconds since epoch.
    int64_t now_ms();

} // namespace safemon