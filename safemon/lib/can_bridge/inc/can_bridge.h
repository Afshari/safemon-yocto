#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include "ican_reader.h"
#include "frame_store.h"

class CanBridge
{
public:
    CanBridge(std::unique_ptr<ICanReader>  can_reader,
              std::unique_ptr<IFrameStore> frame_store);

    void start();
    void stop();

    ~CanBridge() { stop(); }

    // non-copyable
    CanBridge(const CanBridge&)            = delete;
    CanBridge& operator=(const CanBridge&) = delete;

private:
    void run();
    std::string build_frame_string(const CanFrame& frame) const;

    std::unique_ptr<ICanReader>  can_reader_;
    std::unique_ptr<IFrameStore> frame_store_;
    std::thread                  thread_;
    std::atomic<bool>            running_;
};