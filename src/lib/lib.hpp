#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <vector>

template <typename... T>
[[noreturn]] inline void implementMe(const T&... args) {
  (((void)args), ...);
  throw std::runtime_error("Implement me!");
}

template <typename PixelType>
class Image {
 public:
  Image(int wid, int hei, PixelType clearColor)
      : data_(wid * hei, clearColor), width_(wid), height_(hei) {}

  int width() const { return width_; }
  int height() const { return height_; }

  PixelType& at(int x, int y) {
    assert(x >= 0 && x < width_);
    assert(y >= 0 && y < height_);
    size_t pixel_index = y * width() + x;
    return data_[pixel_index];
  }

  const PixelType& at(int x, int y) const {
    assert(x >= 0 && x < width_);
    assert(y >= 0 && y < height_);
    size_t pixel_index = y * width() + x;
    return data_[pixel_index];
  }

 private:
  std::vector<PixelType> data_;
  int width_;
  int height_;
};

struct Triangle {
  std::array<glm::vec3, 3> v;
  std::array<glm::vec3, 3> n;
  std::array<glm::vec2, 3> uv;
};

void drawLine(glm::ivec2 s, glm::ivec2 e, glm::vec3 col, Image<glm::vec3>& img);

void drawWireframeTriangle(glm::ivec2 a, glm::ivec2 b, glm::ivec2 c,
                           glm::vec3 col, Image<glm::vec3>& img);

void drawLine(glm::vec2 s, glm::vec2 e, glm::vec3 col, Image<glm::vec3>& img);

void drawWireframeTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec3 col,
                           Image<glm::vec3>& img);

void writeImg(const Image<glm::vec3>& img, const char* path, bool flipY = true);

std::vector<Triangle> loadObj(const char* path);

struct Camera {
  glm::vec3 position{0.0f, 0.0f, 3.5f};
  glm::vec3 target{0.0f, 0.0f, 0.0f};
  glm::vec3 up{0.0f, 1.0f, 0.0f};
  float fovY = glm::radians(45.0f);
  float aspect = 1.0f;
  float zNear = 0.1f;
  float zFar = 100.0f;

  glm::mat4 view() const { return glm::lookAt(position, target, up); }
  glm::mat4 projection() const {
    return glm::perspective(fovY, aspect, zNear, zFar);
  }
};

struct Texture {
  int w = 0, h = 0;
  std::vector<glm::vec3> data;
  bool load(const char* path);
  glm::vec3 sample(glm::vec2 uv) const;
};

struct Uniforms {  // параметры, общие для всех вершин и пикселей
  glm::mat4 model{1.0f}, view{1.0f}, proj{1.0f};
  glm::vec3 lightPos{2.0f, 2.0f, 4.0f};
  glm::vec3 lightColor{1.0f};
  glm::vec3 cameraPos{0.0f, 0.0f, 3.5f};
  float ambient = 0.15f;
  float specStrength = 0.4f;
  float shininess = 32.0f;
  bool cullBack = true;
  const Texture* texture = nullptr;
};

struct VertexOut {  // что вершинный шейдер отдаёт растеризатору
  glm::vec4 clip;
  glm::vec3 worldPos;
  glm::vec3 normal;
  glm::vec2 uv;
};

VertexOut vertexShader(const Uniforms& u, const glm::vec3& pos,
                       const glm::vec3& n, const glm::vec2& uv);
glm::vec3 fragmentShader(const Uniforms& u, const VertexOut& in);
void drawTriangle(const Uniforms& u, const VertexOut& a, const VertexOut& b,
                  const VertexOut& c, Image<glm::vec3>& color,
                  Image<float>& depth);
void renderMesh(const std::vector<Triangle>& tris, const Uniforms& u,
                Image<glm::vec3>& color, Image<float>& depth);
