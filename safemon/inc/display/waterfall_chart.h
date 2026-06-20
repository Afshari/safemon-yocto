#pragma once
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include "waterfall_data.h"

namespace Safemon {

    class WaterfallChart
    {
    public:
        void Init(const WaterfallData& data);
        void Render(const glm::mat4& view, const glm::mat4& proj);
        void Shutdown();

    private:
        void BuildGeometry(const WaterfallData& data);
        void BuildShaders();

        struct RGB { float r, g, b; };
        RGB ScoreToColor(float score) const;

        GLuint m_vao          = 0;
        GLuint m_vbo          = 0;
        GLuint m_program      = 0;
        GLint  m_loc_mvp      = -1;

        int m_days            = 0;
        int m_buckets_per_day = 0;

        static constexpr float Z_SPACING  = 1.0f;
        static constexpr float MAX_HEIGHT = 4.0f;
    };

} // namespace Safemon