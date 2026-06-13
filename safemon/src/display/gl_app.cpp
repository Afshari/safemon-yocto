#include "gl_app.h"
#include <dirent.h>
#include <linux/input.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Shaders
// ---------------------------------------------------------------------------
static const char* VERT_SRC = R"(
    attribute vec2 a_pos;
    attribute vec3 a_color;
    varying vec3 v_color;
    uniform float u_angle;
    void main() {
        float c = cos(u_angle);
        float s = sin(u_angle);
        vec2 rotated = vec2(
            a_pos.x * c - a_pos.y * s,
            a_pos.x * s + a_pos.y * c
        );
        gl_Position = vec4(rotated, 0.0, 1.0);
        v_color = a_color;
    }
)";

static const char* FRAG_SRC = R"(
    precision mediump float;
    varying vec3 v_color;
    void main() {
        gl_FragColor = vec4(v_color, 1.0);
    }
)";

// ---------------------------------------------------------------------------
static const char* RECT_VERT_SRC = R"(
    attribute vec2 a_pos;
    uniform vec2 u_pos;
    uniform vec2 u_size;
    uniform vec2 u_screen;
    void main() {
        vec2 pixel = u_pos + a_pos * u_size;
        vec2 ndc = (pixel / u_screen) * 2.0 - 1.0;
        ndc.y = -ndc.y;
        gl_Position = vec4(ndc, 0.0, 1.0);
    }
)";

static const char* RECT_FRAG_SRC = R"(
    precision mediump float;
    uniform vec3 u_color;
    void main() {
        gl_FragColor = vec4(u_color, 1.0);
    }
)";

// Simple 5x7 bitmap font — index 0 = space (32), stored sequentially
static const uint8_t FONT[][7] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 space
    {0x04,0x04,0x04,0x04,0x00,0x04,0x00}, // 33 !
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 34 "
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 35 #
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 36 $
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 37 %
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 38 &
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 39 '
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 40 (
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 41 )
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 42 *
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 43 +
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x08}, // 44 ,
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}, // 45 -
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}, // 46 .
    {0x01,0x02,0x02,0x04,0x08,0x08,0x10}, // 47 /
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 48 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 49 1
    {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F}, // 50 2
    {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}, // 51 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 52 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 53 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 54 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 55 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 56 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 57 9
    {0x00,0x04,0x00,0x00,0x04,0x00,0x00}, // 58 :
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 59 ;
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 60 
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 61 =
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 62 >
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 63 ?
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 64 @
    {0x04,0x0A,0x11,0x11,0x1F,0x11,0x11}, // 65 A
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // 66 B
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // 67 C
    {0x1E,0x09,0x09,0x09,0x09,0x09,0x1E}, // 68 D
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // 69 E
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // 70 F
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, // 71 G
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // 72 H
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // 73 I
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}, // 74 J
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // 75 K
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // 76 L
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // 77 M
    {0x11,0x11,0x19,0x15,0x13,0x11,0x11}, // 78 N
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // 79 O
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // 80 P
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // 81 Q
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // 82 R
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // 83 S
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // 84 T
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // 85 U
    {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, // 86 V
    {0x11,0x11,0x15,0x15,0x15,0x1B,0x11}, // 87 W
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // 88 X
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // 89 Y
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // 90 Z
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 91 [
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 92 backslash
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 93 ]
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 94 ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 95 _
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 96 `
    {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F}, // 97 a
    {0x10,0x10,0x1E,0x11,0x11,0x11,0x1E}, // 98 b
    {0x00,0x00,0x0E,0x11,0x10,0x11,0x0E}, // 99 c
    {0x01,0x01,0x0F,0x11,0x11,0x11,0x0F}, // 100 d
    {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E}, // 101 e
    {0x06,0x09,0x08,0x1C,0x08,0x08,0x08}, // 102 f
    {0x00,0x0F,0x11,0x11,0x0F,0x01,0x0E}, // 103 g
    {0x10,0x10,0x1E,0x11,0x11,0x11,0x11}, // 104 h
    {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E}, // 105 i
    {0x02,0x00,0x06,0x02,0x02,0x12,0x0C}, // 106 j
    {0x10,0x10,0x11,0x12,0x1C,0x12,0x11}, // 107 k
    {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E}, // 108 l
    {0x00,0x00,0x1A,0x15,0x15,0x11,0x11}, // 109 m
    {0x00,0x00,0x1E,0x11,0x11,0x11,0x11}, // 110 n
    {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E}, // 111 o
    {0x00,0x1E,0x11,0x11,0x1E,0x10,0x10}, // 112 p
    {0x00,0x0F,0x11,0x11,0x0F,0x01,0x01}, // 113 q
    {0x00,0x00,0x16,0x19,0x10,0x10,0x10}, // 114 r
    {0x00,0x00,0x0E,0x10,0x0E,0x01,0x1E}, // 115 s
    {0x08,0x08,0x1C,0x08,0x08,0x09,0x06}, // 116 t
    {0x00,0x00,0x11,0x11,0x11,0x11,0x0F}, // 117 u
    {0x00,0x00,0x11,0x11,0x11,0x0A,0x04}, // 118 v
    {0x00,0x00,0x11,0x15,0x15,0x15,0x0A}, // 119 w
    {0x00,0x00,0x11,0x0A,0x04,0x0A,0x11}, // 120 x
    {0x00,0x11,0x11,0x0F,0x01,0x11,0x0E}, // 121 y
    {0x00,0x00,0x1F,0x02,0x04,0x08,0x1F}, // 122 z
};


// ---------------------------------------------------------------------------
// Geometry  one triangle, interleaved pos(x,y) + color(r,g,b)
// ---------------------------------------------------------------------------
const float VERTS[] = {
//   x      y      r     g     b
     0.0f,  0.8f,  1.0f, 0.2f, 0.2f,   // top    red
    -0.7f, -0.5f,  0.2f, 1.0f, 0.2f,   // left   green
     0.7f, -0.5f,  0.2f, 0.2f, 1.0f,   // right  blue
};
const int VERTS_COUNT = 3;
const int VERTS_SIZE = sizeof(VERTS);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
GLuint compile_shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "[gl] Shader compile error: " << log << "\n";
    }
    return s;
}

// ---------------------------------------------------------------------------
GLuint build_program() {
    GLuint vert = compile_shader(GL_VERTEX_SHADER,   VERT_SRC);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "[gl] Program link error: " << log << "\n";
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

// ---------------------------------------------------------------------------
GLuint build_rect_program() {
    GLuint vert = compile_shader(GL_VERTEX_SHADER,   RECT_VERT_SRC);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, RECT_FRAG_SRC);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

// ---------------------------------------------------------------------------
void draw_rect(GLuint prog, float x, float y, float w, float h,
               float r, float g, float b,
               float screen_w, float screen_h) {
    // unit quad — two triangles
    static const float QUAD[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
    };

    glUseProgram(prog);

    GLint loc_pos    = glGetAttribLocation(prog,  "a_pos");
    GLint loc_upos   = glGetUniformLocation(prog, "u_pos");
    GLint loc_usize  = glGetUniformLocation(prog, "u_size");
    GLint loc_screen = glGetUniformLocation(prog, "u_screen");
    GLint loc_color  = glGetUniformLocation(prog, "u_color");

    glUniform2f(loc_upos,   x, y);
    glUniform2f(loc_usize,  w, h);
    glUniform2f(loc_screen, screen_w, screen_h);
    glUniform3f(loc_color,  r, g, b);

    glEnableVertexAttribArray(loc_pos);
    glVertexAttribPointer(loc_pos, 2, GL_FLOAT, GL_FALSE, 0, QUAD);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(loc_pos);
}

void set_rotation(GLuint prog, float angle) {
    GLint loc = glGetUniformLocation(prog, "u_angle");
    glUniform1f(loc, angle);
}

int open_keyboard() {
    DIR* dir = opendir("/dev/input");
    if (!dir) return -1;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // only process eventX files
        if (strncmp(entry->d_name, "event", 5) != 0) continue;

        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);

        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        // check if it has EV_KEY capability
        uint8_t evbits[EV_MAX / 8 + 1] = {};
        ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits);
        if (!(evbits[EV_KEY / 8] & (1 << (EV_KEY % 8)))) {
            close(fd);
            continue;
        }

        // check for multiple keys that only a real keyboard has
        uint8_t keybits[KEY_MAX / 8 + 1] = {};
        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);

        bool has_esc   = keybits[KEY_ESC   / 8] & (1 << (KEY_ESC   % 8));
        bool has_a     = keybits[KEY_A     / 8] & (1 << (KEY_A     % 8));
        bool has_space = keybits[KEY_SPACE / 8] & (1 << (KEY_SPACE % 8));

        if (has_esc && has_a && has_space) {
            std::cout << "[input] Keyboard found: /dev/input/"
                    << entry->d_name << "\n";
            closedir(dir);
            return fd;
        }
        
        close(fd);
    }

    closedir(dir);
    return -1;
}

// ---------------------------------------------------------------------------
GLuint build_font_texture() {
    const int CHAR_COUNT = 91;
    const int CHAR_W     = 5;
    const int CHAR_H     = 7;
    const int TEX_W      = CHAR_COUNT * CHAR_W;  // 455
    const int TEX_H      = CHAR_H;               // 7

    // build a TEX_W x TEX_H RGBA pixel buffer
    uint8_t pixels[TEX_W * TEX_H * 4] = {};

    for (int ci = 0; ci < CHAR_COUNT; ci++) {
        for (int row = 0; row < CHAR_H; row++) {
            for (int col = 0; col < CHAR_W; col++) {
                bool on = FONT[ci][row] & (0x10 >> col);
                int px = ci * CHAR_W + col;
                int py = row;
                int idx = (py * TEX_W + px) * 4;
                uint8_t val = on ? 255 : 0;
                pixels[idx + 0] = val;  // R
                pixels[idx + 1] = val;  // G
                pixels[idx + 2] = val;  // B
                pixels[idx + 3] = val;  // A
            }
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 TEX_W, TEX_H, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

static const char* TEXT_VERT_SRC = R"(
    attribute vec2 a_pos;
    attribute vec2 a_uv;
    uniform vec2 u_pos;
    uniform vec2 u_size;
    uniform vec2 u_screen;
    varying vec2 v_uv;
    void main() {
        vec2 pixel = u_pos + a_pos * u_size;
        vec2 ndc = (pixel / u_screen) * 2.0 - 1.0;
        ndc.y = -ndc.y;
        gl_Position = vec4(ndc, 0.0, 1.0);
        v_uv = a_uv;
    }
)";

static const char* TEXT_FRAG_SRC = R"(
    precision mediump float;
    uniform sampler2D u_tex;
    uniform vec3 u_color;
    varying vec2 v_uv;
    void main() {
        float alpha = texture2D(u_tex, v_uv).a;
        gl_FragColor = vec4(u_color, alpha);
    }
)";

GLuint build_text_program() {
    GLuint vert = compile_shader(GL_VERTEX_SHADER,   TEXT_VERT_SRC);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, TEXT_FRAG_SRC);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

void draw_text_gl(GLuint text_prog, GLuint font_tex,
                  float x, float y, const char* text,
                  float r, float g, float b,
                  float scale,
                  float screen_w, float screen_h) {
    const int CHAR_COUNT = 91;
    const int CHAR_W     = 5;
    const float UV_CHAR_W = 1.0f / CHAR_COUNT;

    glUseProgram(text_prog);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_tex);
    glUniform1i(glGetUniformLocation(text_prog, "u_tex"), 0);
    glUniform3f(glGetUniformLocation(text_prog, "u_color"), r, g, b);
    glUniform2f(glGetUniformLocation(text_prog, "u_screen"), screen_w, screen_h);
    glUniform2f(glGetUniformLocation(text_prog, "u_size"),
                CHAR_W * scale, 7.0f * scale);

    GLint loc_pos = glGetAttribLocation(text_prog, "a_pos");
    GLint loc_uv  = glGetAttribLocation(text_prog, "a_uv");

    float cx = x;
    for (int i = 0; text[i] != '\0'; i++) {
        uint8_t c = (uint8_t)text[i];
        if (c < 32 || c > 122) { cx += 6 * scale; continue; }
        int ci = c - 32;

        float u0 = ci       * UV_CHAR_W;
        float u1 = (ci + 1) * UV_CHAR_W;

        float verts[] = {
            // x     y     u   v
            0.0f, 0.0f,  u0, 0.0f,
            1.0f, 0.0f,  u1, 0.0f,
            0.0f, 1.0f,  u0, 1.0f,
            1.0f, 0.0f,  u1, 0.0f,
            1.0f, 1.0f,  u1, 1.0f,
            0.0f, 1.0f,  u0, 1.0f,
        };

        glUniform2f(glGetUniformLocation(text_prog, "u_pos"), cx, y);

        glEnableVertexAttribArray(loc_pos);
        glEnableVertexAttribArray(loc_uv);
        glVertexAttribPointer(loc_pos, 2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), verts);
        glVertexAttribPointer(loc_uv,  2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), verts + 2);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        cx += 6 * scale;
    }

    glDisableVertexAttribArray(loc_pos);
    glDisableVertexAttribArray(loc_uv);
    glDisable(GL_BLEND);
}