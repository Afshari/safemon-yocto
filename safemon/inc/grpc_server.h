#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <hiredis/hiredis.h>
#include "config.h"

class GrpcServer {
public:
    GrpcServer(const SafemonConfig& cfg, const std::string& listen_address);
    ~GrpcServer();

    void start();
    void stop();

private:
    void run();

    SafemonConfig cfg_;
    std::string   listen_address_;
    std::thread   thread_;
    std::atomic<bool> running_;
};