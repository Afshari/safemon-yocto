#pragma once
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include "camera.h"
#include "text_renderer.h"

namespace Safemon {

    class AxisGizmo
    {
    public:
        void Init();
        void Render(const Camera& camera, TextRenderer& textRenderer,
                   int screen_w, int screen_h);
        void Shutdown();

    private:
        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        GLuint m_program_id = 0;

        static constexpr int VIEWPORT_SIZE   = 120;
        static constexpr int VIEWPORT_MARGIN = 20;
    };

} // namespace Safemon