#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include "config.h"
#include "redis_client.h"

class GrpcServer
{
public:
    GrpcServer(const SafemonConfig& cfg,
               const std::string&   listen_address,
               std::unique_ptr<IRedisClient> redis_client);

    ~GrpcServer() { stop(); }

    void start();
    void stop();

    // non-copyable
    GrpcServer(const GrpcServer&)            = delete;
    GrpcServer& operator=(const GrpcServer&) = delete;

private:
    void run();

    SafemonConfig                  cfg_;
    std::string                    listen_address_;
    std::unique_ptr<IRedisClient>  redis_client_;
    std::thread                    thread_;
    std::atomic<bool>              running_;
};