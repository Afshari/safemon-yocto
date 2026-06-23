#pragma once
#include <string>
#include "ican_reader.h"

class CanReader : public ICanReader
{
public:
    explicit CanReader(const std::string& interface);
    ~CanReader() override;

    // non-copyable
    CanReader(const CanReader&)            = delete;
    CanReader& operator=(const CanReader&) = delete;

    bool open()              override;
    bool read(CanFrame& frame) override;
    void close()             override;

private:
    std::string interface_;
    int         socket_fd_ = -1;
};