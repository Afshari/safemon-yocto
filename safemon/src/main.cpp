#include <iostream>
#include <iomanip>
#include <csignal>
#include <hiredis/hiredis.h>
#include "can_reader.h"

static volatile bool running = true;

void signal_handler(int) {
    running = false;
}

int main() {
    std::signal(SIGINT, signal_handler);

    // Connect to Redis
    redisContext* ctx = redisConnect("127.0.0.1", 6379);
    if (!ctx || ctx->err) {
        std::cerr << "Redis connection failed" << std::endl;
        return 1;
    }
    std::cout << "Connected to Redis!" << std::endl;

    // Open CAN interface
    CanReader reader("vcan0");
    if (!reader.open()) {
        std::cerr << "Failed to open vcan0" << std::endl;
        return 1;
    }

    std::cout << "Listening on vcan0... (Ctrl+C to stop)" << std::endl;

    while (running) {
        CanFrame frame;
        if (!reader.read(frame)) continue;

        // Print frame
        std::cout << "ID: 0x" << std::hex << std::setw(3)
                  << std::setfill('0') << frame.id
                  << " LEN: " << std::dec << (int)frame.len
                  << " DATA: ";
        for (int i = 0; i < frame.len; i++) {
            std::cout << std::hex << std::setw(2)
                      << std::setfill('0')
                      << (int)frame.data[i] << " ";
        }
        std::cout << std::endl;

        // Store in Redis
        redisCommand(ctx, "LPUSH safemon:can:frames %x#%02x%02x%02x%02x",
            frame.id,
            frame.data[0], frame.data[1],
            frame.data[2], frame.data[3]);
    }

    std::cout << "Shutting down..." << std::endl;
    redisFree(ctx);
    return 0;
}