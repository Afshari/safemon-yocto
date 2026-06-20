#include "text_renderer.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>

namespace Safemon {

    static constexpr int ATLAS_SIZE = 512;
    static constexpr int FIRST_CHAR = 32;
    static constexpr int NUM_CHARS  = 96;

    struct TextRenderer::BakedAtlas
    {
        GLuint texture = 0;
        stbtt_bakedchar chars[NUM_CHARS];
    };

    static const char* TEXT_VERT_SRC = R"(#version 300 es
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;

uniform mat4 u_projection;

out vec2 v_uv;

void main() {
    gl_Position = u_projection * vec4(a_pos, 0.0, 1.0);
    v_uv = a_uv;
}
)";

    static const char* TEXT_FRAG_SRC = R"(#version 300 es
precision mediump float;

in vec2 v_uv;
out vec4 frag_color;

uniform sampler2D u_atlas;
uniform vec3 u_color;

void main() {
    float alpha = texture(u_atlas, v_uv).r;
    frag_color = vec4(u_color, alpha);
}
)";

    bool TextRenderer::Init(const std::string& font_path)
    {
        std::ifstream file(font_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "[TextRenderer] Cannot open font: " << font_path << "\n";
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        m_font_buffer.resize(size);
        if (!file.read(reinterpret_cast<char*>(m_font_buffer.data()), size)) {
            std::cerr << "[TextRenderer] Failed to read font file\n";
            return false;
        }

        GLuint vert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 1, &TEXT_VERT_SRC, nullptr);
        glCompileShader(vert);

        GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &TEXT_FRAG_SRC, nullptr);
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
            std::cerr << "[TextRenderer] Shader link error: " << log << "\n";
            return false;
        }

        glDeleteShader(vert);
        glDeleteShader(frag);

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        std::cout << "[TextRenderer] Initialized with " << font_path << "\n";
        return true;
    }

    bool TextRenderer::AddSize(const std::string& size_name, float pixel_height)
    {
        BakedAtlas* atlas = new BakedAtlas();
        std::vector<unsigned char> bitmap(ATLAS_SIZE * ATLAS_SIZE);

        int result = stbtt_BakeFontBitmap(
            m_font_buffer.data(), 0,
            pixel_height,
            bitmap.data(), ATLAS_SIZE, ATLAS_SIZE,
            FIRST_CHAR, NUM_CHARS,
            atlas->chars
        );

        if (result <= 0) {
            std::cerr << "[TextRenderer] Bake failed for size '" << size_name << "'\n";
            delete atlas;
            return false;
        }

        glGenTextures(1, &atlas->texture);
        glBindTexture(GL_TEXTURE_2D, atlas->texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                     ATLAS_SIZE, ATLAS_SIZE, 0,
                     GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_atlases[size_name] = atlas;
        std::cout << "[TextRenderer] Baked size '" << size_name
                  << "' at " << pixel_height << "px\n";
        return true;
    }

    void TextRenderer::Begin(int screen_w, int screen_h)
    {
        m_screen_w = screen_w;
        m_screen_h = screen_h;

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

    void TextRenderer::DrawText(float x, float y, const std::string& text,
                                const glm::vec3& color, const std::string& size_name)
    {
        auto it = m_atlases.find(size_name);
        if (it == m_atlases.end()) {
            std::cerr << "[TextRenderer] Unknown size: " << size_name << "\n";
            return;
        }
        BakedAtlas* atlas = it->second;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas->texture);
        glUniform1i(glGetUniformLocation(m_program_id, "u_atlas"), 0);
        glUniform3f(glGetUniformLocation(m_program_id, "u_color"),
                   color.r, color.g, color.b);

        float cursor_x = x;
        float cursor_y = y;

        for (char c : text) {
            if (c < FIRST_CHAR || c >= FIRST_CHAR + NUM_CHARS) continue;

            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(
                atlas->chars, ATLAS_SIZE, ATLAS_SIZE,
                c - FIRST_CHAR,
                &cursor_x, &cursor_y,
                &q, 1
            );

            float verts[6][4] = {
                { q.x0, q.y0, q.s0, q.t0 },
                { q.x1, q.y0, q.s1, q.t0 },
                { q.x1, q.y1, q.s1, q.t1 },

                { q.x0, q.y0, q.s0, q.t0 },
                { q.x1, q.y1, q.s1, q.t1 },
                { q.x0, q.y1, q.s0, q.t1 },
            };

            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    void TextRenderer::End()
    {
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glUseProgram(0);
    }

    void TextRenderer::Shutdown()
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteProgram(m_program_id);

        for (auto& [name, atlas] : m_atlases) {
            glDeleteTextures(1, &atlas->texture);
            delete atlas;
        }
        m_atlases.clear();
    }

} // namespace Safemon