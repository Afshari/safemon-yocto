#include "grpc_server.h"
#include "../proto/fault.grpc.pb.h"
#include "../proto/fault.pb.h"
#include <grpcpp/grpcpp.h>
#include <hiredis/hiredis.h>
#include <iostream>
#include <chrono>
#include <thread>

// ---------------------------------------------------------------------------
// FaultServiceImpl -- gRPC service implementation
// Polls Redis every 500ms, streams FaultEvent to connected clients on change.
// ---------------------------------------------------------------------------

class FaultServiceImpl final : public safemon::FaultService::Service {
public:
    explicit FaultServiceImpl(const SafemonConfig& cfg) : cfg_(cfg) {}

    grpc::Status StreamFaults(
        grpc::ServerContext*                    context,
        const safemon::StreamRequest*           request,
        grpc::ServerWriter<safemon::FaultEvent>* writer) override
    {
        // Each streaming RPC call gets its own Redis connection
        redisContext* redis = redisConnect(
            cfg_.redis_host.c_str(), cfg_.redis_port);
        if (!redis || redis->err) {
            std::cerr << "[grpc] Redis connection failed\n";
            return grpc::Status(grpc::StatusCode::INTERNAL,
                                "Redis connection failed");
        }

        std::string last_status;

        while (!context->IsCancelled()) {
            // Read current fault status from Redis
            redisReply* reply = static_cast<redisReply*>(
                redisCommand(redis, "GET safemon:faults:current"));

            std::string current_status;
            if (reply && reply->type == REDIS_REPLY_STRING) {
                current_status = reply->str;
            } else {
                current_status = "UNKNOWN";
            }
            if (reply) freeReplyObject(reply);

            // Stream event only when status changes
            if (current_status != last_status) {
                last_status = current_status;

                safemon::FaultEvent event;
                event.set_status(current_status);
                event.set_source("safemon-app");

                auto now = std::chrono::system_clock::now();
                auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();
                event.set_timestamp(ms);

                // signature field left empty here -- filled in Phase 5
                event.set_signature("");

                if (!writer->Write(event)) {
                    break; // client disconnected
                }

                std::cout << "[grpc] Streamed fault: " << current_status << "\n";
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        redisFree(redis);
        return grpc::Status::OK;
    }

private:
    SafemonConfig cfg_;
};

// ---------------------------------------------------------------------------
// GrpcServer
// ---------------------------------------------------------------------------

GrpcServer::GrpcServer(const SafemonConfig& cfg,
                       const std::string&   listen_address)
    : cfg_(cfg)
    , listen_address_(listen_address)
    , running_(false)
{}

GrpcServer::~GrpcServer() {
    stop();
}

void GrpcServer::start() {
    running_ = true;
    thread_  = std::thread(&GrpcServer::run, this);
}

void GrpcServer::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void GrpcServer::run() {
    FaultServiceImpl service(cfg_);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(listen_address_,
                             grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server) {
        std::cerr << "[grpc] Failed to start server on "
                  << listen_address_ << "\n";
        return;
    }

    std::cout << "[grpc] Server listening on " << listen_address_ << "\n";

    // Wait until stop() sets running_ to false
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    server->Shutdown();
}