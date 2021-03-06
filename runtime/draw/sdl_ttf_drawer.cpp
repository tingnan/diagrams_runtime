// Copyright 2015 Native Client Authors

#include <array>
#include <iostream>
#include <iomanip>

#include "draw/sdl_ttf_drawer.h"
#include "draw/camera.h"

namespace {
const char kFragShaderSource[] =
    "precision mediump float;\n"
    "varying vec4 v_color;\n"
    "varying vec2 tex_coord;\n"
    "uniform sampler2D tex_data;\n"
    "void main() {\n"
    "  vec4 alpha = vec4(1.0, 1.0, 1.0, texture2D(tex_data, tex_coord).a);\n"
    "  gl_FragColor = v_color * alpha;\n"
    "}\n";

const char kVertShaderSource[] =
    "uniform mat4 u_mvp;\n"
    "attribute vec4 position;\n"
    "attribute vec4 color;\n"
    "varying vec4 v_color;\n"
    "varying vec2 tex_coord;\n"
    "void main() {\n"
    "  gl_Position = u_mvp * vec4(position.xy, 0.0, 1.0);\n"
    "  tex_coord = position.zw;\n"
    "  v_color = vec4(color.xyz, 1.0);\n"
    "}\n";

const std::array<GLfloat, 16> kQuad = {{-0.5, 0.5, 0.0, 0.0, 0.5, 0.5, 1.0, 0.0,
                                        0.5, -0.5, 1.0, 1.0, -0.5, -0.5, 0.0,
                                        1.0}};

const std::array<GLfloat, 16> kQuadColor = {{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                                             1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                                             1.0, 1.0}};

const std::array<GLuint, 6> kQuadIndices = {{0, 1, 2, 2, 3, 0}};

SDL_Surface *StringToSDLSurface(const char *message, TTF_Font *font) {
  SDL_Color color = {255, 255, 255, 255};
  SDL_Surface *ttf_surface = TTF_RenderText_Blended(font, message, color);
  if (ttf_surface == nullptr) {
    std::cerr << "TTF_RenderText: " << SDL_GetError() << std::endl;
    return nullptr;
  }
  return ttf_surface;
}

}  // namespace

namespace diagrammar {

TextDrawer::TextDrawer(TTF_Font *font) : font_(font) {
  GenBuffers();
  program_.pid = CreateGLProgram(kVertShaderSource, kFragShaderSource);
  program_.u_mvp = glGetUniformLocation(program_.pid, "u_mvp");
  program_.color = glGetAttribLocation(program_.pid, "color");
  program_.vertex = glGetAttribLocation(program_.pid, "position");
  program_.texture = glGetUniformLocation(program_.pid, "tex_data");
}

void TextDrawer::GenBuffers() {
  glGenTextures(1, &text_buffer_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, text_buffer_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenBuffers(1, &vert_buffer_);

  glGenBuffers(1, &vert_indice_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vert_indice_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * kQuadIndices.size(),
               kQuadIndices.data(), GL_STATIC_DRAW);
  glGenBuffers(1, &vert_color_);
  glBindBuffer(GL_ARRAY_BUFFER, vert_color_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * kQuadColor.size(),
               kQuadColor.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void TextDrawer::Draw(const std::string &text, const Vector2f &pos,
                      Camera *camera) {
  GLProgram program = program_;
  SDL_Surface *surface = StringToSDLSurface(text.c_str(), font_);
  if (surface == nullptr) {
    return;
  }

  glUseProgram(program.pid);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, text_buffer_);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, surface->pixels);
  // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glUniform1i(program.texture, 0);

  // vertex
  glBindBuffer(GL_ARRAY_BUFFER, vert_buffer_);
  std::array<GLfloat, 16> vert_array = kQuad;
  for (size_t i = 0; i < 16; i += 4) {
    vert_array[i + 0] = vert_array[i + 0] * float(surface->w) + pos(0);
    vert_array[i + 1] = vert_array[i + 1] * float(surface->h) + pos(1);
  }
  glBufferData(GL_ARRAY_BUFFER, vert_array.size() * sizeof(GLfloat),
               vert_array.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(program.vertex, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(program.vertex);

  // color
  glBindBuffer(GL_ARRAY_BUFFER, vert_color_);
  glVertexAttribPointer(program.color, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(program.color);

  // text location
  Matrix4f u_mvp(Matrix4f::Identity());
  u_mvp = camera->GetViewProjection() * u_mvp;
  glUniformMatrix4fv(program.u_mvp, 1, false, u_mvp.data());

  // scale
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vert_indice_);
  glDrawElements(GL_TRIANGLES, kQuadIndices.size(), GL_UNSIGNED_INT, 0);
  // glDrawArrays(GL_LINE_LOOP, 0, 4);

  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(0);
  SDL_FreeSurface(surface);
  glUseProgram(0);
}

}  // namespace diagrammar
