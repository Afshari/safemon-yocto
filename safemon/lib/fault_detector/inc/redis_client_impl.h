#pragma once
#include <string>
#include <hiredis/hiredis.h>
#include "redis_client.h"
#include "frame_store.h"

class RedisClient : public IRedisClient, public IFrameStore
{
public:
    RedisClient(const std::string& host, int port);
    ~RedisClient() override;

    // non-copyable
    RedisClient(const RedisClient&)            = delete;
    RedisClient& operator=(const RedisClient&) = delete;

    // IRedisClient
    std::string get_latest_frame() override;
    void publish_fault(const std::string& level,
                       const std::string& message) override;

    // IFrameStore
    void push_frame(const std::string& frame) override;

    std::string get_fault_status() override;

    bool is_connected() const { return redis_ && !redis_->err; }

private:
    redisContext* redis_ = nullptr;
};