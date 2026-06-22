#pragma once
#include <string>

class IFrameStore
{
public:
    virtual ~IFrameStore() = default;

    virtual void push_frame(const std::string& frame) = 0;
};