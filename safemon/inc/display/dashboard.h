#pragma once
#include <glm/glm.hpp>
#include <string>
#include "glass_panel.h"
#include "text_renderer.h"

namespace Safemon {

    struct DashboardState
    {
        bool        can_ok          = true;
        bool        redis_ok        = true;
        std::string fault_code      = "ERROR : TIMEOUT";
        std::string fault_detail1   = "No new frame received 5.2s";
        std::string fault_detail2   = "Last known ID 0x456";
        std::string last_frame      = "0x456 - 8B";
        std::string last_frame_time = "07:14:52.118";
        long        frame_count     = 1248;
        std::string uptime          = "02:41:09";
        std::string clock           = "15:21:08";
        std::string footer_machine  = "RPI4";
        std::string footer_build    = "BUILD 2026.06.20";
    };

    class Dashboard
    {
    public:
        void Init(TextRenderer* textRenderer, float design_scale);
        void Render(const DashboardState& state, float time_seconds,
                   int actual_screen_w, int actual_screen_h);
        void Shutdown();

    private:
        void DrawConnectionPanels(const DashboardState& state);
        void DrawFaultPanel(const DashboardState& state);
        void DrawTelemetryPanels();

        void DrawHeaderText(const DashboardState& state);
        void DrawConnectionText(const DashboardState& state);
        void DrawFaultText(const DashboardState& state);
        void DrawTelemetryText(const DashboardState& state);
        void DrawFooterText(const DashboardState& state);

        // scale-aware draw helpers
        void Panel(float x, float y, float w, float h, const GlassPanel::Style& style);
        void Circle(float x, float y, float r, const glm::vec4& color);
        void Text(float x, float y, const std::string& s, const glm::vec4& color, const std::string& size);

        GlassPanel    m_panel;
        TextRenderer* m_text = nullptr;

        float m_scale = 1.0f;
        float m_offset_x = 0.0f;
        float m_offset_y = 0.0f;

        static constexpr int SCREEN_W = 800;
        static constexpr int SCREEN_H = 480;

        static constexpr float CONN_Y  = 72.0f;
        static constexpr float CONN_H  = 52.0f;
        static constexpr float FAULT_Y = 140.0f;
        static constexpr float FAULT_H = 96.0f;
        static constexpr float TELEM_Y = 252.0f;
        static constexpr float TELEM_H = 64.0f;
    };

} // namespace Safemon