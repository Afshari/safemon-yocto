#pragma once

#include <cstdint>
#include <string>

// ARGB color helpers
namespace Color {
    constexpr uint32_t Black   = 0xFF000000;
    constexpr uint32_t White   = 0xFFFFFFFF;
    constexpr uint32_t Red     = 0xFFFF0000;
    constexpr uint32_t Green   = 0xFF00FF00;
    constexpr uint32_t Blue    = 0xFF0000FF;
    constexpr uint32_t Yellow  = 0xFFFFFF00;
    constexpr uint32_t Gray    = 0xFF444444;
    constexpr uint32_t DarkBg  = 0xFF1A1A2E;
}

class Framebuffer {
public:
    Framebuffer(uint32_t* map, uint32_t width,
                uint32_t height, uint32_t pitch);

    void fill(uint32_t color);
    void draw_rect(int x, int y, int w, int h, uint32_t color);
    void draw_text(int x, int y, const std::string& text,
                   uint32_t color, int scale = 1);
    void draw_pixel(int x, int y, uint32_t color);

    uint32_t width()  const { return width_; }
    uint32_t height() const { return height_; }

private:
    uint32_t* map_;
    uint32_t  width_;
    uint32_t  height_;
    uint32_t  pitch_;  // bytes per row
};