#pragma once
#include <GLES2/gl2.h>
#include <iostream>

extern const float VERTS[];
extern const int   VERTS_COUNT;
extern const int VERTS_SIZE;

GLuint compile_shader(GLenum type, const char* src);
GLuint build_program();
void set_rotation(GLuint prog, float angle);
int open_keyboard();

GLuint build_rect_program();
void draw_rect(GLuint prog, float x, float y, float w, float h,
               float r, float g, float b,
               float screen_w, float screen_h);