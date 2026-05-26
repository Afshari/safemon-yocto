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