#pragma once
#include <cstdint>

struct CanFrame {
    uint32_t id  = 0;
    uint8_t  len = 0;
    uint8_t  data[8] = {};
};

class ICanReader
{
public:
    virtual ~ICanReader() = default;

    virtual bool open()             = 0;
    virtual bool read(CanFrame& frame) = 0;
    virtual void close()            = 0;
};