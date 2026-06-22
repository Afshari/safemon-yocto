#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include "config.h"
#include "fault_detector.h"
#include "ecdsa_verify_file.h"
#include "grpc_server.h"
#include "redis_client_impl.h"
#include "ican_reader.h"
#include "can_reader.h"
#include "can_bridge.h"
#include "frame_store.h"

static volatile bool running = true;

void signal_handler(int)
{
    running = false;
}

int main()
{
    std::signal(SIGINT, signal_handler);

    SafemonConfig cfg = load_config("/etc/safemon/safemon.conf");

    if (!ecdsa::verify_file("/etc/safemon/safemon.conf",
                            "/etc/safemon/pki/safemon.pub"))
    {
        std::cerr << "[startup] Config verification failed -- aborting\n";
        return 1;
    }

    FaultDetector fault_detector(
        cfg,
        std::make_unique<RedisClient>(cfg.redis_host, cfg.redis_port)
    );
    fault_detector.start();

    GrpcServer grpc_server(cfg, "0.0.0.0:50051");
    grpc_server.start();

    CanBridge can_bridge(
        static_cast<std::unique_ptr<ICanReader>>(std::make_unique<CanReader>("vcan0")),
        static_cast<std::unique_ptr<IFrameStore>>(
            std::make_unique<RedisClient>(cfg.redis_host, cfg.redis_port))
    );
    can_bridge.start();

    std::cout << "[safemon] Running. Ctrl+C to stop.\n";

    while (running)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "[safemon] Shutting down...\n";
    can_bridge.stop();
    fault_detector.stop();
    grpc_server.stop();
    return 0;
}