#pragma once
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

namespace Safemon {

    class GlassPanel
    {
    public:
        void Init();
        void Shutdown();

        void Begin(int screen_w, int screen_h, float time_seconds);

        struct Style {
            glm::vec4 fill_color    = { 0.13f, 0.24f, 0.38f, 0.30f };
            glm::vec4 border_color  = { 0.47f, 0.75f, 1.0f,  0.35f };
            float     corner_radius = 14.0f;
            float     border_width  = 1.5f;
            bool      pulse         = false;
        };

        void DrawPanel(float x, float y, float w, float h, const Style& style);
        void DrawCircle(float cx, float cy, float radius, const glm::vec4& color, float glow = 0.0f);

        void End();

    private:
        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        GLuint m_program_id = 0;

        int   m_screen_w = 0;
        int   m_screen_h = 0;
        float m_time = 0.0f;
    };

} // namespace Safemon