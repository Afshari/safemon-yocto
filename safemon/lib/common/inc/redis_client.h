#pragma once
#include <string>

class IRedisClient
{
public:
    virtual ~IRedisClient() = default;

    virtual std::string get_latest_frame()  = 0;
    virtual std::string get_fault_status()  = 0;
    virtual long get_frame_count() = 0;
    virtual void publish_fault(const std::string& level,
                               const std::string& message) = 0;
};