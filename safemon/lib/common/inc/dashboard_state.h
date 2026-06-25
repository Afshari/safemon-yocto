#pragma once
#include <string>

struct DashboardState
{
    bool        can_ok          = true;
    bool        redis_ok        = true;
    std::string fault_code      = "APP DOWN";
    std::string fault_detail1   = "No new frame received";
    std::string fault_detail2   = "Last known ID unknown";
    std::string last_frame      = "---";
    std::string last_frame_time = "00:00:00.000";
    long        frame_count     = 0;
    std::string uptime          = "00:00:00";
    std::string clock           = "00:00:00";
    std::string footer_machine  = "---";
    std::string footer_build    = "---";
};