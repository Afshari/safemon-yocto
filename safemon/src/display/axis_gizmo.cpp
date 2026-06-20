#include "axis_gizmo.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace Safemon {

    static const char* GIZMO_VERT_SRC = R"(#version 300 es
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_color;

uniform mat4 u_mvp;
out vec3 v_color;

void main() {
    gl_Position = u_mvp * vec4(a_pos, 1.0);
    v_color = a_color;
}
)";

    static const char* GIZMO_FRAG_SRC = R"(#version 300 es
precision mediump float;

in vec3 v_color;
out vec4 frag_color;

void main() {
    frag_color = vec4(v_color, 1.0);
}
)";

    void AxisGizmo::Init()
    {
        float verts[] = {
            // X axis - bright red
            0,0,0,  1.0f, 0.25f, 0.25f,
            1,0,0,  1.0f, 0.25f, 0.25f,
            // Y axis - bright green
            0,0,0,  0.3f, 1.0f, 0.4f,
            0,1,0,  0.3f, 1.0f, 0.4f,
            // Z axis - bright cyan-blue
            0,0,0,  0.3f, 0.6f, 1.0f,
            0,0,1,  0.3f, 0.6f, 1.0f,
        };

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        GLuint vert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 1, &GIZMO_VERT_SRC, nullptr);
        glCompileShader(vert);

        GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &GIZMO_FRAG_SRC, nullptr);
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
            std::cerr << "[AxisGizmo] Shader link error: " << log << "\n";
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void AxisGizmo::Render(const Camera& camera, TextRenderer& textRenderer,
                          int screen_w, int screen_h)
    {
        glm::mat4 view = glm::mat4(1.0f);
        view[0] = glm::vec4(camera.GetRight(),   0.0f);
        view[1] = glm::vec4(camera.GetUp(),      0.0f);
        view[2] = glm::vec4(-camera.GetForward(),0.0f);
        view = glm::transpose(view);

        view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -2.5f)) * view;

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 mvp = proj * view;

        int vp_x = VIEWPORT_MARGIN;
        int vp_y = VIEWPORT_MARGIN;
        glViewport(vp_x, vp_y, VIEWPORT_SIZE, VIEWPORT_SIZE);

        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_program_id);
        glUniformMatrix4fv(glGetUniformLocation(m_program_id, "u_mvp"),
                          1, GL_FALSE, glm::value_ptr(mvp));

        glBindVertexArray(m_vao);
        glLineWidth(3.0f);
        glDrawArrays(GL_LINES, 0, 6);
        glLineWidth(1.0f);
        glBindVertexArray(0);
        glUseProgram(0);

        glViewport(0, 0, screen_w, screen_h);

        auto projectLabel = [&](const glm::vec3& world, const char* label, const glm::vec3& color) {
            glm::vec4 clip = mvp * glm::vec4(world, 1.0f);
            if (clip.w <= 0.0f) return;
            glm::vec3 ndc = glm::vec3(clip) / clip.w;

            float local_x = (ndc.x * 0.5f + 0.5f) * VIEWPORT_SIZE;
            float local_y = (1.0f - (ndc.y * 0.5f + 0.5f)) * VIEWPORT_SIZE;

            float sx = vp_x + local_x;
            float sy = (screen_h - vp_y - VIEWPORT_SIZE) + local_y;

            textRenderer.DrawText(sx, sy, label, color, "small");
        };

        textRenderer.Begin(screen_w, screen_h);
        projectLabel(glm::vec3(1.3f, 0, 0), "X", glm::vec3(1.0f, 0.25f, 0.25f));
        projectLabel(glm::vec3(0, 1.3f, 0), "Y", glm::vec3(0.3f, 1.0f, 0.4f));
        projectLabel(glm::vec3(0, 0, 1.3f), "Z", glm::vec3(0.3f, 0.6f, 1.0f));
        textRenderer.End();
    }

    void AxisGizmo::Shutdown()
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteProgram(m_program_id);
    }

} // namespace Safemon