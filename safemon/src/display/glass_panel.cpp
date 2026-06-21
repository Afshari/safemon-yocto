#include "glass_panel.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace Safemon {

    static const char* PANEL_VERT_SRC = R"(#version 300 es
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_local;

uniform mat4  u_projection;
uniform vec2  u_pos;
uniform vec2  u_size;

out vec2 v_local;
out vec2 v_uv;

void main() {
    vec2 pixel = u_pos + a_pos * u_size;
    gl_Position = u_projection * vec4(pixel, 0.0, 1.0);
    v_local = a_local;
    v_uv = a_pos;
}
)";

    static const char* PANEL_FRAG_SRC = R"(#version 300 es
precision highp float;

in vec2 v_local;
in vec2 v_uv;
out vec4 frag_color;

uniform vec2  u_size;
uniform vec4  u_fill_color;
uniform vec4  u_border_color;
uniform float u_corner_radius;
uniform float u_border_width;
uniform float u_time;
uniform int   u_pulse;
uniform int   u_mode; // 0 = rounded rect, 1 = circle/glow dot

float sdRoundBox(vec2 p, vec2 half_size, float radius) {
    vec2 q = abs(p) - half_size + radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

void main() {
    vec2 half_size = u_size * 0.5;
    vec2 p = v_local * half_size;

    if (u_mode == 1) {
        float r = half_size.x;
        float dist = length(p) - r;

        float core = 1.0 - smoothstep(-1.5, 1.5, dist);
        float glow = 1.0 - smoothstep(0.0, r * 1.8, max(dist, 0.0));

        vec3 color = u_fill_color.rgb;
        float alpha = max(core * u_fill_color.a, glow * u_fill_color.a * 0.35);

        if (alpha < 0.01) discard;
        frag_color = vec4(color, alpha);
        return;
    }

    float dist = sdRoundBox(p, half_size, u_corner_radius);
    float fill_mask = 1.0 - smoothstep(-1.5, 1.5, dist);
    float border_mask = smoothstep(-u_border_width - 1.5, -u_border_width + 1.5, dist)
                      - smoothstep(-1.5, 1.5, dist);
    border_mask = clamp(border_mask, 0.0, 1.0);

    float sheen = smoothstep(half_size.y, -half_size.y * 0.2, p.y) * 0.06;

    vec3 color = u_fill_color.rgb + vec3(sheen);
    float alpha = u_fill_color.a * fill_mask;

    color = mix(color, u_border_color.rgb, border_mask);
    alpha = max(alpha, u_border_color.a * border_mask);

    if (u_pulse == 1) {
        float t = fract(u_time * 0.45);
        float ring_radius = mix(half_size.x * 0.3, half_size.x * 1.4, t);
        float ring_dist = abs(length(p) - ring_radius);
        float ring_mask = (1.0 - smoothstep(0.0, 3.0, ring_dist)) * (1.0 - t);
        color = mix(color, u_border_color.rgb, ring_mask);
        alpha = max(alpha, ring_mask * 0.8);
    }

    if (alpha < 0.01) discard;
    frag_color = vec4(color, alpha);
}
)";

    void GlassPanel::Init()
    {
        float verts[] = {
            0.0f, 0.0f,  -1.0f, -1.0f,
            1.0f, 0.0f,   1.0f, -1.0f,
            1.0f, 1.0f,   1.0f,  1.0f,

            0.0f, 0.0f,  -1.0f, -1.0f,
            1.0f, 1.0f,   1.0f,  1.0f,
            0.0f, 1.0f,  -1.0f,  1.0f,
        };

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        GLuint vert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 1, &PANEL_VERT_SRC, nullptr);
        glCompileShader(vert);

        GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &PANEL_FRAG_SRC, nullptr);
        glCompileShader(frag);

        m_program_id = glCreateProgram();
        glAttachShader(m_program_id, vert);
        glAttachShader(m_program_id, frag);
        glLinkProgram(m_program_id);

        GLint ok = 0;
        glGetProgramiv(m_program_id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetProgramInfoLog(m_program_id, 512, nullptr, log);
            std::cerr << "[GlassPanel] Shader link error: " << log << "\n";
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void GlassPanel::Begin(int screen_w, int screen_h, float time_seconds)
    {
        m_screen_w = screen_w;
        m_screen_h = screen_h;
        m_time = time_seconds;

        glUseProgram(m_program_id);

        glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(screen_w),
                                    static_cast<float>(screen_h), 0.0f);
        glUniformMatrix4fv(glGetUniformLocation(m_program_id, "u_projection"),
                          1, GL_FALSE, glm::value_ptr(proj));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        glBindVertexArray(m_vao);
    }

    void GlassPanel::DrawPanel(float x, float y, float w, float h, const Style& style)
    {
        glUniform2f(glGetUniformLocation(m_program_id, "u_pos"), x, y);
        glUniform2f(glGetUniformLocation(m_program_id, "u_size"), w, h);
        glUniform4f(glGetUniformLocation(m_program_id, "u_fill_color"),
                   style.fill_color.r, style.fill_color.g, style.fill_color.b, style.fill_color.a);
        glUniform4f(glGetUniformLocation(m_program_id, "u_border_color"),
                   style.border_color.r, style.border_color.g, style.border_color.b, style.border_color.a);
        glUniform1f(glGetUniformLocation(m_program_id, "u_corner_radius"), style.corner_radius);
        glUniform1f(glGetUniformLocation(m_program_id, "u_border_width"), style.border_width);
        glUniform1f(glGetUniformLocation(m_program_id, "u_time"), m_time);
        glUniform1i(glGetUniformLocation(m_program_id, "u_pulse"), style.pulse ? 1 : 0);
        glUniform1i(glGetUniformLocation(m_program_id, "u_mode"), 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void GlassPanel::DrawCircle(float cx, float cy, float radius, const glm::vec4& color, float glow)
    {
        glUniform2f(glGetUniformLocation(m_program_id, "u_pos"), cx - radius, cy - radius);
        glUniform2f(glGetUniformLocation(m_program_id, "u_size"), radius * 2.0f, radius * 2.0f);
        glUniform4f(glGetUniformLocation(m_program_id, "u_fill_color"), color.r, color.g, color.b, color.a);
        glUniform4f(glGetUniformLocation(m_program_id, "u_border_color"), 0, 0, 0, 0);
        glUniform1f(glGetUniformLocation(m_program_id, "u_corner_radius"), 0.0f);
        glUniform1f(glGetUniformLocation(m_program_id, "u_border_width"), 0.0f);
        glUniform1f(glGetUniformLocation(m_program_id, "u_time"), m_time);
        glUniform1i(glGetUniformLocation(m_program_id, "u_pulse"), 0);
        glUniform1i(glGetUniformLocation(m_program_id, "u_mode"), 1);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void GlassPanel::End()
    {
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glUseProgram(0);
    }

    void GlassPanel::Shutdown()
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteProgram(m_program_id);
    }

} // namespace Safemon