#include <iostream>
#include <iomanip>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <hiredis/hiredis.h>
#include <thread>
#include "can_reader.h"
#include "config.h"
#include "fault_detector.h"
#include "ecdsa_verify_file.h"

static volatile bool running = true;

void signal_handler(int)
{
    running = false;
}

int main()
{
    std::signal(SIGINT, signal_handler);
    SafemonConfig cfg = load_config("/etc/safemon/safemon.conf");

    // Verify config signature before proceeding
    if (!ecdsa::verify_file("/etc/safemon/safemon.conf",
                            "/etc/safemon/pki/safemon.pub"))
    {
        std::cerr << "[startup] Config verification failed -- aborting\n";
        return 1;
    }

    // Connect to Redis
    redisContext *ctx = redisConnect(cfg.redis_host.c_str(), cfg.redis_port);
    if (!ctx || ctx->err)
    {
        std::cerr << "Redis connection failed" << std::endl;
        return 1;
    }
    std::cout << "Connected to Redis!" << std::endl;

    FaultDetector fault_detector(ctx, cfg);
    fault_detector.start();

    // Open CAN interface
    CanReader reader("vcan0");
    if (!reader.open())
    {
        std::cerr << "Failed to open vcan0" << std::endl;
        return 1;
    }

    std::cout << "Listening on vcan0... (Ctrl+C to stop)" << std::endl;

    while (running)
    {
        CanFrame frame;
        if (!reader.read(frame))
            continue;

        // Print frame
        std::cout << "ID: 0x" << std::hex << std::setw(3)
                  << std::setfill('0') << frame.id
                  << " LEN: " << std::dec << (int)frame.len
                  << " DATA: ";
        for (int i = 0; i < frame.len; i++)
        {
            std::cout << std::hex << std::setw(2)
                      << std::setfill('0')
                      << (int)frame.data[i] << " ";
        }
        std::cout << std::endl;

        // Build frame string
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%03X#", frame.id);
        for (int i = 0; i < frame.len; i++)
        {
            char byte[3];
            std::snprintf(byte, sizeof(byte), "%02X", frame.data[i]);
            std::strncat(buf, byte, sizeof(buf) - strlen(buf) - 1);
        }

        // Push as a single string
        redisCommand(ctx, "LPUSH safemon:can:frames %s", buf);
    }

    std::cout << "Shutting down..." << std::endl;
    redisFree(ctx);
    fault_detector.stop();
    return 0;
}