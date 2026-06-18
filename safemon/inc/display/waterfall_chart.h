#pragma once
#include <GLES3/gl3.h>
#include <vector>
#include "waterfall_data.h"

namespace Safemon {

    class WaterfallChart
    {
    public:
        void Init(const WaterfallData& data, uint32_t screen_w, uint32_t screen_h);
        void Render();
        void Shutdown();

    private:
        void BuildGeometry(const WaterfallData& data);
        void BuildShaders();
        void SetupCamera();

        // color helper
        struct RGB { float r, g, b; };
        RGB ScoreToColor(float score) const;

        GLuint m_vao            = 0;
        GLuint m_vbo            = 0;
        GLuint m_program        = 0;
        GLint  m_loc_mvp        = -1;

        int m_days              = 0;
        int m_buckets_per_day   = 0;

        uint32_t m_screen_w     = 0;
        uint32_t m_screen_h     = 0;

        // camera matrices stored as flat float arrays for GL
        float m_view[16];
        float m_proj[16];

        static constexpr float Z_SPACING  = 1.0f;
        static constexpr float MAX_HEIGHT = 4.0f;
    };

} // namespace Safemon