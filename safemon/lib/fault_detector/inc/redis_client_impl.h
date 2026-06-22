#pragma once
#include <string>
#include <hiredis/hiredis.h>
#include "redis_client.h"

class RedisClient : public IRedisClient
{
public:
    RedisClient(const std::string& host, int port);
    ~RedisClient() override;

    // non-copyable
    RedisClient(const RedisClient&)            = delete;
    RedisClient& operator=(const RedisClient&) = delete;

    std::string get_latest_frame() override;
    void publish_fault(const std::string& level,
                       const std::string& message) override;

    bool is_connected() const { return redis_ && !redis_->err; }

private:
    redisContext* redis_ = nullptr;
};