#include "waterfall_chart.h"
#include <GLES3/gl3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>

namespace Safemon {

// ---------------------------------------------------------------------------
// Color stops - matching JavaScript version
// ---------------------------------------------------------------------------
static const float COLOR_STOPS[][4] = {
    { 0.00f, 0.078f, 0.137f, 0.235f },  // DarkIndigo  #14233C
    { 0.15f, 0.149f, 0.294f, 0.576f },  // Indigo      #264B93
    { 0.30f, 0.314f, 0.780f, 0.878f },  // VividSkyBlue #50C7E0
    { 0.50f, 0.404f, 0.741f, 0.686f },  // VividGreen  #67BDAF
    { 0.70f, 0.863f, 0.475f, 0.412f },  // MutedRed    #DC7969
    { 0.90f, 0.957f, 0.518f, 0.125f },  // VividOrange #F48420
    { 1.00f, 0.925f, 0.059f, 0.424f },  // VividPink   #EC0F6C
};
static const int NUM_STOPS = 7;


// ---------------------------------------------------------------------------
// Shader source
// ---------------------------------------------------------------------------

static const char* VERT_SRC = R"(#version 300 es
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_color;

uniform mat4 u_mvp;

out vec3 v_color;

void main()
{
    gl_Position = u_mvp * vec4(a_pos, 1.0);
    v_color = a_color;
}
)";

static const char* FRAG_SRC = R"(#version 300 es
precision mediump float;

in  vec3 v_color;
out vec4 frag_color;

void main()
{
    frag_color = vec4(v_color, 1.0);
}
)";

// ---------------------------------------------------------------------------
// WaterfallChart implementation
// ---------------------------------------------------------------------------

WaterfallChart::RGB WaterfallChart::ScoreToColor(float score) const
{
    const float t = score / 60.0f < 1.0f ? score / 60.0f : 1.0f;

    for (int i = 0; i < NUM_STOPS - 1; i++) {
        const float lo = COLOR_STOPS[i][0];
        const float hi = COLOR_STOPS[i + 1][0];
        if (t <= hi) {
            const float f = (t - lo) / (hi - lo);
            return {
                COLOR_STOPS[i][1] + f * (COLOR_STOPS[i+1][1] - COLOR_STOPS[i][1]),
                COLOR_STOPS[i][2] + f * (COLOR_STOPS[i+1][2] - COLOR_STOPS[i][2]),
                COLOR_STOPS[i][3] + f * (COLOR_STOPS[i+1][3] - COLOR_STOPS[i][3]),
            };
        }
    }
    return { COLOR_STOPS[NUM_STOPS-1][1],
             COLOR_STOPS[NUM_STOPS-1][2],
             COLOR_STOPS[NUM_STOPS-1][3] };
}

void WaterfallChart::BuildShaders()
{
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, 512, nullptr, log);
            std::cerr << "[WaterfallChart] Shader error: " << log << "\n";
        }
        return s;
    };

    GLuint vert = compile(GL_VERTEX_SHADER,   VERT_SRC);
    GLuint frag = compile(GL_FRAGMENT_SHADER, FRAG_SRC);

    m_program = glCreateProgram();
    glAttachShader(m_program, vert);
    glAttachShader(m_program, frag);
    glLinkProgram(m_program);

    GLint ok = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(m_program, 512, nullptr, log);
        std::cerr << "[WaterfallChart] Link error: " << log << "\n";
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    m_loc_mvp = glGetUniformLocation(m_program, "u_mvp");
}

void WaterfallChart::BuildGeometry(const WaterfallData& data)
{
    m_days            = data.days;
    m_buckets_per_day = data.buckets_per_day;

    // 6 floats per vertex: x, y, z, r, g, b
    std::vector<float> vertices;
    vertices.reserve(m_days * m_buckets_per_day * 6);

    for (int d = 0; d < m_days; d++) {
        const auto& day = data.day_list[d];
        const float z   = static_cast<float>(d) * Z_SPACING;

        for (int b = 0; b < m_buckets_per_day; b++) {
            const float score = day.buckets[b].score;
            const float x     = static_cast<float>(b) / 25.0f;
            const float y     = (score / 5.0f);

            const RGB color = ScoreToColor(score);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(color.r);
            vertices.push_back(color.g);
            vertices.push_back(color.b);
        }
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(float),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // position: location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // color: location 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}


void WaterfallChart::Init(const WaterfallData& data)
{
    BuildShaders();
    BuildGeometry(data);

    std::cout << "[WaterfallChart] Initialized\n";
}

void WaterfallChart::Render(const glm::mat4& view, const glm::mat4& proj)
{
    glUseProgram(m_program);

    glLineWidth(1.0f); // chart lines stay thin, reset from gizmo's wider lines

    glm::mat4 mvp = proj * view;
    glUniformMatrix4fv(m_loc_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(m_vao);
    for (int d = 0; d < m_days; d++) {
        glDrawArrays(GL_LINE_STRIP,
                     d * m_buckets_per_day,
                     m_buckets_per_day);
    }
    glBindVertexArray(0);
    glUseProgram(0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        std::cerr << "[gl] Error: " << err << "\n";
}

void WaterfallChart::Shutdown()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteProgram(m_program);
}

} // namespace Safemon