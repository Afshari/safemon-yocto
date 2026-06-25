#include "grpc_server.h"
#include "fault_event_builder.h"
#include <grpcpp/grpcpp.h>
#include "fault.grpc.pb.h"
#include "fault.pb.h"
#include <iostream>
#include <thread>
#include <chrono>

// ---------------------------------------------------------------------------
// FaultServiceImpl
// ---------------------------------------------------------------------------

class FaultServiceImpl final : public safemon::FaultService::Service
{
public:
    explicit FaultServiceImpl(IRedisClient* redis_client)
        : redis_client_(redis_client)
    {}

    grpc::Status StreamFaults(
        grpc::ServerContext*                     context,
        const safemon::StreamRequest*            request,
        grpc::ServerWriter<safemon::FaultEvent>* writer) override
    {
        std::string last_status;

        while (!context->IsCancelled()) {
            std::string current_status = redis_client_->get_fault_status();
            if (current_status.empty())
                current_status = "UNKNOWN";

            safemon::FaultEventData event_data;
            if (safemon::build_fault_event(current_status, last_status, event_data)) {
                last_status = current_status;

                safemon::FaultEvent proto_event;
                proto_event.set_status(event_data.status);
                proto_event.set_source(event_data.source);
                proto_event.set_timestamp(event_data.timestamp_ms);
                proto_event.set_signature("");

                if (!writer->Write(proto_event)) {
                    break; // client disconnected
                }

                std::cout << "[grpc] Streamed fault: " << current_status << "\n";
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        return grpc::Status::OK;
    }

private:
    IRedisClient* redis_client_;
};

// ---------------------------------------------------------------------------
// GrpcServer
// ---------------------------------------------------------------------------

GrpcServer::GrpcServer(const SafemonConfig& cfg,
                       const std::string&   listen_address,
                       std::unique_ptr<IRedisClient> redis_client)
    : cfg_(cfg)
    , listen_address_(listen_address)
    , redis_client_(std::move(redis_client))
    , running_(false)
{}

void GrpcServer::start()
{
    running_ = true;
    thread_  = std::thread(&GrpcServer::run, this);
}

void GrpcServer::stop()
{
    running_ = false;
    if (thread_.joinable())
        thread_.join();
}

void GrpcServer::run()
{
    FaultServiceImpl service(redis_client_.get());

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

    while (running_)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    server->Shutdown();
}