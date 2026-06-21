#pragma once
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace Safemon {

    class TextRenderer
    {
    public:
        bool Init(const std::string& font_path);
        void Shutdown();

        bool AddSize(const std::string& size_name, float pixel_height);

        void Begin(int screen_w, int screen_h);
        void DrawText(float x, float y, const std::string& text,
                     const glm::vec3& color, const std::string& size_name);
        void End();

    private:
        struct BakedAtlas;

        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        GLuint m_program_id = 0;

        std::vector<unsigned char> m_font_buffer;

        int m_screen_w = 0;
        int m_screen_h = 0;

        std::unordered_map<std::string, BakedAtlas*> m_atlases;
    };

} // namespace Safemon