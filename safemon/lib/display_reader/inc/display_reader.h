#pragma once
#include <string>
#include <memory>
#include "dashboard_state.h"
#include "redis_client.h"

class DisplayStateReader
{
public:
    explicit DisplayStateReader(std::unique_ptr<IRedisClient> redis_client);

    // reads all Redis keys and populates a DashboardState
    // redis_ok parameter reflects whether the connection is alive
    DashboardState read(bool& redis_ok);

private:
    std::string extract_can_id(const std::string& frame) const;

    std::unique_ptr<IRedisClient> redis_client_;

    // tracks last seen frame to compute elapsed time
    std::string last_seen_frame_;
    time_t      last_change_time_ = 0;
};