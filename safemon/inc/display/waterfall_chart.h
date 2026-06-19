#pragma once
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "waterfall_data.h"

namespace Safemon {

    class WaterfallChart
    {
    public:
        void Init(const WaterfallData& data, uint32_t screen_w, uint32_t screen_h);
        void Render();
        void RenderLabels(GLuint text_prog, GLuint font_tex);
        void Shutdown();

    private:
        void BuildGeometry(const WaterfallData& data);
        void BuildShaders();
        void SetupCamera();

        struct RGB { float r, g, b; };
        RGB ScoreToColor(float score) const;

        GLuint m_vao          = 0;
        GLuint m_vbo          = 0;
        GLuint m_program      = 0;
        GLint  m_loc_mvp      = -1;

        int m_days            = 0;
        int m_buckets_per_day = 0;

        uint32_t m_screen_w   = 0;
        uint32_t m_screen_h   = 0;

        glm::mat4 m_view;
        glm::mat4 m_proj;

        static constexpr float Z_SPACING  = 1.0f;
        static constexpr float MAX_HEIGHT = 4.0f;
    };

} // namespace Safemon