#include "can_bridge.h"
#include <iostream>
#include <cstdio>
#include <cstring>

CanBridge::CanBridge(std::unique_ptr<ICanReader>  can_reader,
                     std::unique_ptr<IFrameStore> frame_store)
    : can_reader_(std::move(can_reader))
    , frame_store_(std::move(frame_store))
    , running_(false)
{
}

void CanBridge::start()
{
    if (!can_reader_->open()) {
        std::cerr << "[can_bridge] Failed to open CAN interface\n";
        return;
    }

    running_ = true;
    thread_  = std::thread(&CanBridge::run, this);
    std::cout << "[can_bridge] Started\n";
}

void CanBridge::stop()
{
    running_ = false;
    can_reader_->close();
    if (thread_.joinable())
        thread_.join();
    std::cout << "[can_bridge] Stopped\n";
}

void CanBridge::run()
{
    while (running_) {
        CanFrame frame;
        if (!can_reader_->read(frame))
            continue;

        std::string frame_str = build_frame_string(frame);
        frame_store_->push_frame(frame_str);
    }
}

std::string CanBridge::build_frame_string(const CanFrame& frame) const
{
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%03X#", frame.id);
    for (int i = 0; i < frame.len; i++) {
        char byte[3];
        std::snprintf(byte, sizeof(byte), "%02X", frame.data[i]);
        std::strncat(buf, byte, sizeof(buf) - std::strlen(buf) - 1);
    }
    return std::string(buf);
}