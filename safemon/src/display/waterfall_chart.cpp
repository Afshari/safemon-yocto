#include "waterfall_chart.h"
#include <GLES3/gl3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include "gl_app.h"

namespace Safemon {

static bool ProjectToScreen(const glm::vec3& world,
                             const glm::mat4& mvp,
                             float screen_w, float screen_h,
                             float& out_x, float& out_y)
{
    glm::vec4 clip = mvp * glm::vec4(world, 1.0f);
    if (clip.w <= 0.0f) return false;

    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    out_x = (ndc.x * 0.5f + 0.5f) * screen_w;
    out_y = (1.0f - (ndc.y * 0.5f + 0.5f)) * screen_h;
    return true;
}

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
// Minimal math helpers - no GLM on embedded
// ---------------------------------------------------------------------------

static void mat4_identity(float m[16])
{
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void mat4_multiply(float out[16], const float a[16], const float b[16])
{
    float tmp[16] = {};
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            for (int k = 0; k < 4; k++)
                tmp[r * 4 + c] += a[r * 4 + k] * b[k * 4 + c];
    memcpy(out, tmp, 16 * sizeof(float));
}

static void mat4_perspective(float m[16],
                              float fovy_rad, float aspect,
                              float near, float far)
{
    memset(m, 0, 16 * sizeof(float));
    float f = 1.0f / tanf(fovy_rad * 0.5f);
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = -(far + near) / (far - near);
    m[11] = -1.0f;
    m[14] = -(2.0f * far * near) / (far - near);
}

static void mat4_lookat(float m[16],
                         float ex, float ey, float ez,
                         float cx, float cy, float cz,
                         float ux, float uy, float uz)
{
    // forward = normalize(center - eye)
    float fx = cx - ex, fy = cy - ey, fz = cz - ez;
    float fl = sqrtf(fx*fx + fy*fy + fz*fz);
    fx /= fl; fy /= fl; fz /= fl;

    // right = normalize(forward x up)
    float rx = fy*uz - fz*uy;
    float ry = fz*ux - fx*uz;
    float rz = fx*uy - fy*ux;
    float rl = sqrtf(rx*rx + ry*ry + rz*rz);
    rx /= rl; ry /= rl; rz /= rl;

    // up = right x forward
    float vx = ry*fz - rz*fy;
    float vy = rz*fx - rx*fz;
    float vz = rx*fy - ry*fx;

    memset(m, 0, 16 * sizeof(float));
    m[0]  = rx;  m[4]  = ry;  m[8]  = rz;  m[12] = -(rx*ex + ry*ey + rz*ez);
    m[1]  = vx;  m[5]  = vy;  m[9]  = vz;  m[13] = -(vx*ex + vy*ey + vz*ez);
    m[2]  = -fx; m[6]  = -fy; m[10] = -fz; m[14] = (fx*ex + fy*ey + fz*ez);
    m[15] = 1.0f;
}

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


void WaterfallChart::Init(const WaterfallData& data,
                           uint32_t screen_w, uint32_t screen_h)
{
    m_screen_w = screen_w;
    m_screen_h = screen_h;

    BuildShaders();
    BuildGeometry(data);
    SetupCamera();

    std::cout << "[WaterfallChart] Initialized\n";
}

void WaterfallChart::SetupCamera()
{
    const float aspect = static_cast<float>(m_screen_w) /
                         static_cast<float>(m_screen_h);

    const float cx = (static_cast<float>(m_buckets_per_day) / 100.0f) * 0.5f;
    const float cz = (static_cast<float>(m_days) * Z_SPACING) * 0.5f;

    m_view = glm::lookAt(
        glm::vec3(cx + 40.0f, 15.0f, cz + 70.0f),
        glm::vec3(cx + 20.0f,  0.0f, cz),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    m_proj = glm::perspective(
        glm::radians(45.0f), aspect, 0.01f, 1000.0f
    );
}

void WaterfallChart::Render()
{
    glUseProgram(m_program);

    glm::mat4 mvp = m_proj * m_view;
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

void WaterfallChart::RenderLabels(GLuint text_prog, GLuint font_tex)
{
    glm::mat4 mvp = m_proj * m_view;

    for (int d = 0; d < m_days; d += 5) {
        float z = static_cast<float>(d) * Z_SPACING;
        float sx, sy;
        if (ProjectToScreen(glm::vec3(-1.0f, 0.0f, z), mvp,
                            static_cast<float>(m_screen_w),
                            static_cast<float>(m_screen_h), sx, sy)) {
            std::string label = "D" + std::to_string(d);
            draw_text_gl(text_prog, font_tex, sx, sy, label.c_str(),
                        0.6f, 0.8f, 1.0f, 1.5f,
                        static_cast<float>(m_screen_w),
                        static_cast<float>(m_screen_h));
        }
    }
}

void WaterfallChart::Shutdown()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteProgram(m_program);
}

} // namespace Safemon