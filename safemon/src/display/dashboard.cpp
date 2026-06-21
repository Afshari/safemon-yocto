#include "dashboard.h"
#include <glm/glm.hpp>
#include <algorithm>

namespace Safemon {

    static const glm::vec4 COL_AMBER       = { 1.0f,  0.69f, 0.0f,  1.0f };
    static const glm::vec4 COL_PURPLE      = { 0.55f, 0.35f, 0.85f, 1.0f };

    struct FaultColors {
        glm::vec4 fill;
        glm::vec4 border;
        glm::vec4 text;
        bool      pulse;
    };

    static FaultColors GetFaultColors(const std::string& fault_code)
    {
        if (fault_code.rfind("OK", 0) == 0) {
            return { {0.10f, 0.28f, 0.18f, 0.28f}, {0.24f, 0.86f, 0.59f, 0.35f}, {0.78f, 1.0f, 0.88f, 1.0f}, false };
        }
        if (fault_code.rfind("WARN", 0) == 0) {
            return { {0.30f, 0.22f, 0.05f, 0.28f}, {1.0f, 0.69f, 0.0f, 0.35f}, {1.0f, 0.92f, 0.75f, 1.0f}, true };
        }
        if (fault_code.rfind("APP DOWN", 0) == 0) {
            return { {0.20f, 0.10f, 0.30f, 0.28f}, {0.55f, 0.35f, 0.85f, 0.35f}, {0.88f, 0.80f, 1.0f, 1.0f}, false };
        }
        // ERROR or unknown - red
        return { {0.30f, 0.10f, 0.13f, 0.28f}, {1.0f, 0.28f, 0.34f, 0.35f}, {1.0f, 0.84f, 0.85f, 1.0f}, true };
    }

    static const glm::vec4 COL_CYAN        = { 0.31f, 0.82f, 1.0f,  1.0f };
    static const glm::vec4 COL_CYAN_SOFT   = { 0.62f, 0.90f, 1.0f,  1.0f };
    static const glm::vec4 COL_GREEN       = { 0.24f, 0.86f, 0.59f, 1.0f };
    static const glm::vec4 COL_RED         = { 1.0f,  0.28f, 0.34f, 1.0f };
    static const glm::vec4 COL_TEXT_BRIGHT = { 0.94f, 0.97f, 1.0f,  1.0f };
    static const glm::vec4 COL_TEXT_MID    = { 0.68f, 0.78f, 0.90f, 1.0f };
    static const glm::vec4 COL_TEXT_DIM    = { 0.50f, 0.60f, 0.74f, 1.0f };

    static const glm::vec4 GLASS_FILL      = { 0.13f, 0.24f, 0.38f, 0.30f };
    static const glm::vec4 GLASS_BORDER    = { 0.47f, 0.75f, 1.0f,  0.18f };
    static const glm::vec4 FAULT_FILL      = { 0.30f, 0.10f, 0.13f, 0.28f };
    static const glm::vec4 FAULT_BORDER    = { 1.0f,  0.28f, 0.34f, 0.35f };

    void Dashboard::Init(TextRenderer* textRenderer, float design_scale)
    {
        m_text = textRenderer;
        m_panel.Init();

        m_text->AddSize("eyebrow", 15.0f * design_scale);
        m_text->AddSize("title",   24.0f * design_scale);
        m_text->AddSize("body",    18.0f * design_scale);
        m_text->AddSize("label",   15.0f * design_scale);
        m_text->AddSize("code",    22.0f * design_scale);
    }

    void Dashboard::DrawConnectionPanels(const DashboardState& state)
    {
        const float gap = 14.0f;
        const float w = (SCREEN_W - 48.0f - gap) / 2.0f;

        GlassPanel::Style style;
        style.fill_color = GLASS_FILL;
        style.border_color = GLASS_BORDER;
        style.corner_radius = 12.0f;

        Panel(24, CONN_Y, w, CONN_H, style);
        Circle(24 + 22, CONN_Y + CONN_H * 0.5f, 5.0f, state.can_ok ? COL_GREEN : COL_RED);

        float x2 = 24 + w + gap;
        Panel(x2, CONN_Y, w, CONN_H, style);
        Circle(x2 + 22, CONN_Y + CONN_H * 0.5f, 5.0f, state.redis_ok ? COL_GREEN : COL_RED);
    }

    void Dashboard::DrawFaultPanel(const DashboardState& state)
    {
        const float x = 24.0f;
        const float w = SCREEN_W - 48.0f;

        FaultColors fc = GetFaultColors(state.fault_code);

        GlassPanel::Style style;
        style.fill_color = fc.fill;
        style.border_color = fc.border;
        style.corner_radius = 14.0f;
        Panel(x, FAULT_Y, w, FAULT_H, style);

        GlassPanel::Style ringStyle;
        ringStyle.fill_color = { 0,0,0,0 };
        ringStyle.border_color = fc.border;
        ringStyle.corner_radius = 23.0f;
        ringStyle.border_width = 2.0f;
        ringStyle.pulse = fc.pulse;
        Panel(x + 24, FAULT_Y + FAULT_H * 0.5f - 23, 46, 46, ringStyle);
    }

    void Dashboard::DrawTelemetryPanels()
    {
        const float gap = 14.0f;
        const float w = (SCREEN_W - 48.0f - gap * 2.0f) / 3.0f;

        GlassPanel::Style style;
        style.fill_color = GLASS_FILL;
        style.border_color = GLASS_BORDER;
        style.corner_radius = 12.0f;

        float x1 = 24.0f;
        float x2 = x1 + w + gap;
        float x3 = x2 + w + gap;

        Panel(x1, TELEM_Y, w, TELEM_H, style);
        Panel(x2, TELEM_Y, w, TELEM_H, style);
        Panel(x3, TELEM_Y, w, TELEM_H, style);
    }

    void Dashboard::DrawHeaderText(const DashboardState& state)
    {
        Text(24, 22, "SAFEMON  -  CAN BUS DIAGNOSTICS", COL_CYAN, "eyebrow");
        Text(24, 42, "STATUS PANEL", COL_TEXT_BRIGHT, "title");
        Text(700, 28, state.clock.c_str(), COL_TEXT_MID, "body");
    }

    void Dashboard::DrawConnectionText(const DashboardState& state)
    {
        const float gap = 14.0f;
        const float w = (SCREEN_W - 48.0f - gap) / 2.0f;

        float label_y = CONN_Y + CONN_H * 0.32f;
        float value_y = CONN_Y + CONN_H * 0.68f;

        Text(24 + 40, label_y, "CAN INTERFACE", COL_TEXT_MID, "label");
        Text(24 + 40, value_y, state.can_ok ? "LINKED" : "ERROR",
            state.can_ok ? COL_GREEN : COL_RED, "body");

        float x2 = 24 + w + gap;
        Text(x2 + 40, label_y, "REDIS STORE", COL_TEXT_MID, "label");
        Text(x2 + 40, value_y, state.redis_ok ? "CONNECTED" : "ERROR",
            state.redis_ok ? COL_GREEN : COL_RED, "body");
    }

    void Dashboard::DrawFaultText(const DashboardState& state)
    {
        const float x = 24.0f;
        const float w = SCREEN_W - 48.0f;

        FaultColors fc = GetFaultColors(state.fault_code);

        float ring_cx = x + 24 + 23;
        float ring_cy = FAULT_Y + FAULT_H * 0.5f;

        const char* glyph = state.fault_code.rfind("OK", 0) == 0 ? "OK" : "!";
        int x_offset = state.fault_code.rfind("OK", 0) == 0 ? 9 : 5;
        Text(ring_cx - x_offset, ring_cy + 7, glyph, fc.text, "code");

        Text(x + 90, FAULT_Y + 24, "ACTIVE FAULT", COL_TEXT_MID, "label");
        Text(x + 90, FAULT_Y + 52, state.fault_code.c_str(), fc.text, "code");

        Text(x + w - 280, FAULT_Y + 28, state.fault_detail1.c_str(), COL_TEXT_MID, "label");
        Text(x + w - 280, FAULT_Y + 46, state.fault_detail2.c_str(), COL_TEXT_MID, "label");
    }

    void Dashboard::DrawTelemetryText(const DashboardState& state)
    {
        const float gap = 14.0f;
        const float w = (SCREEN_W - 48.0f - gap * 2.0f) / 3.0f;

        float x1 = 24.0f;
        float x2 = x1 + w + gap;
        float x3 = x2 + w + gap;

        float label_y = TELEM_Y + TELEM_H * 0.22f;
        float value_y = TELEM_Y + TELEM_H * 0.55f;
        float sub_y   = TELEM_Y + TELEM_H * 0.85f;

        Text(x1 + 14, label_y, "LAST FRAME", COL_TEXT_MID, "label");
        Text(x1 + 14, value_y, state.last_frame.c_str(), COL_CYAN_SOFT, "body");
        Text(x1 + 14, sub_y,   state.last_frame_time.c_str(), COL_TEXT_DIM, "label");

        Text(x2 + 14, label_y, "FRAMES IN QUEUE", COL_TEXT_MID, "label");
        Text(x2 + 14, value_y, std::to_string(state.frame_count).c_str(), COL_TEXT_BRIGHT, "body");

        Text(x3 + 14, label_y, "UPTIME", COL_TEXT_MID, "label");
        Text(x3 + 14, value_y, state.uptime.c_str(), COL_TEXT_BRIGHT, "body");
        Text(x3 + 14, sub_y,   "since last reboot", COL_TEXT_DIM, "label");
    }

    void Dashboard::DrawFooterText(const DashboardState& state)
    {
        Text(24, SCREEN_H - 24, "KERNEL 6.12 PREEMPT  -  SAFEMON v0.2", COL_TEXT_DIM, "label");

        std::string right = state.footer_machine + "  -  " + state.footer_build;
        Text(SCREEN_W - 240, SCREEN_H - 24, right.c_str(), COL_TEXT_DIM, "label");
    }

    void Dashboard::Render(const DashboardState& state, float time_seconds,
                           int actual_screen_w, int actual_screen_h)
    {
        m_scale = std::min(
            static_cast<float>(actual_screen_w) / SCREEN_W,
            static_cast<float>(actual_screen_h) / SCREEN_H
        );
        m_offset_x = (actual_screen_w - SCREEN_W * m_scale) * 0.5f;
        m_offset_y = (actual_screen_h - SCREEN_H * m_scale) * 0.5f;

        m_panel.Begin(actual_screen_w, actual_screen_h, time_seconds);
        DrawConnectionPanels(state);
        DrawFaultPanel(state);
        DrawTelemetryPanels();
        m_panel.End();

        m_text->Begin(actual_screen_w, actual_screen_h);
        DrawHeaderText(state);
        DrawConnectionText(state);
        DrawFaultText(state);
        DrawTelemetryText(state);
        DrawFooterText(state);
        m_text->End();
    }

    void Dashboard::Panel(float x, float y, float w, float h, const GlassPanel::Style& style)
    {
        GlassPanel::Style scaled = style;
        scaled.corner_radius *= m_scale;
        scaled.border_width  *= m_scale;
        m_panel.DrawPanel(m_offset_x + x * m_scale, m_offset_y + y * m_scale,
                          w * m_scale, h * m_scale, scaled);
    }

    void Dashboard::Circle(float x, float y, float r, const glm::vec4& color)
    {
        m_panel.DrawCircle(m_offset_x + x * m_scale, m_offset_y + y * m_scale, r * m_scale, color);
    }

    void Dashboard::Text(float x, float y, const std::string& s, const glm::vec4& color, const std::string& size)
    {
        m_text->DrawText(m_offset_x + x * m_scale, m_offset_y + y * m_scale, s, color, size);
    }

    void Dashboard::Shutdown()
    {
        m_panel.Shutdown();
    }

} // namespace Safemon