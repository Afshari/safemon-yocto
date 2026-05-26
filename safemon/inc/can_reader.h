#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <linux/can.h>

struct CanFrame {
    uint32_t id;
    uint8_t  len;
    uint8_t  data[8];
};

class CanReader {
public:
    explicit CanReader(const std::string& interface);
    ~CanReader();

    bool open();
    void close();
    bool read(CanFrame& frame);

private:
    std::string interface_;
    int socket_fd_;
};