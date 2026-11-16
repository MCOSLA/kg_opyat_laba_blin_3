#include "lib.hpp"

#include <stb_image_write.h>
#include <tiny_obj_loader.h>

#include <algorithm>
#include <cmath>
#include <glm/ext.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iostream>

void drawLine(glm::ivec2 s, glm::ivec2 e, glm::vec3 col,
              Image<glm::vec3>& img) {
  bool steep = false;
  if (std::abs(s.x - e.x) < std::abs(s.y - e.y)) {
    std::swap(s.x, s.y);
    std::swap(e.x, e.y);
    steep = true;
  }
  if (s.x > e.x) {
    std::swap(s.x, e.x);
    std::swap(s.y, e.y);
  }
  int dx = e.x - s.x;
  int dy = e.y - s.y;
  int derror2 = std::abs(dy) * 2;
  int error2 = 0;
  int y = s.y;
  for (int x = s.x; x <= e.x; x++) {
    int px = steep ? y : x;
    int py = steep ? x : y;
    if (px >= 0 && px < img.width() && py >= 0 && py < img.height()) {
      img.at(px, py) = col;
    }
    error2 += derror2;
    if (error2 > dx) {
      y += (e.y > s.y ? 1 : -1);
      error2 -= dx * 2;
    }
  }
}

void drawWireframeTriangle(glm::ivec2 a, glm::ivec2 b, glm::ivec2 c,
                           glm::vec3 col, Image<glm::vec3>& img) {
  drawLine(a, b, col, img);
  drawLine(b, c, col, img);
  drawLine(c, a, col, img);
}
void drawLine(glm::vec2 s, glm::vec2 e, glm::vec3 col, Image<glm::vec3>& img) {
  glm::vec2 max(glm::ivec2(img.width() - 1, img.height() - 1));
  drawLine(glm::ivec2(s * max), glm::ivec2(e * max), col, img);
}

void drawWireframeTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec3 col,
                           Image<glm::vec3>& img) {
  glm::vec2 max(glm::ivec2(img.width() - 1, img.height() - 1));
  drawWireframeTriangle(glm::ivec2(a * max), glm::ivec2(b * max),
                        glm::ivec2(c * max), col, img);
}

void writeImg(const Image<glm::vec3>& img, const char* path, bool flipY) {
  std::vector<std::byte> out;
  out.reserve(3 * img.width() * img.height());
  for (int y = 0; y < img.height(); y++) {
    for (int x = 0; x < img.width(); x++) {
      glm::vec3 col = img.at(x, y);
      out.push_back(static_cast<std::byte>(
          std::lround(std::clamp(col.x, 0.0f, 1.0f) * 255.0f)));
      out.push_back(static_cast<std::byte>(
          std::lround(std::clamp(col.y, 0.0f, 1.0f) * 255.0f)));
      out.push_back(static_cast<std::byte>(
          std::lround(std::clamp(col.z, 0.0f, 1.0f) * 255.0f)));
    }
  }
  stbi_flip_vertically_on_write(flipY ? 1 : 0);
  int res = stbi_write_png(path, img.width(), img.height(), 3, out.data(),
                           3 * img.width());
  if (res) {
    std::cout << "Saved image to: " << path << std::endl;
  } else {
    std::cerr << "Failed to save image to: " << path << std::endl;
  }
}

std::vector<Triangle> loadObj(const char* path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  std::vector<Triangle> verts;
  bool res =
      tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path, NULL, true);
  if (!res) {
    std::cerr << "Failed loading mesh: " << err << std::endl;
    return verts;
  }

  for (auto& shape : shapes) {
    auto& mesh = shape.mesh;
    size_t idx = 0;
    for (size_t i = 0; i < mesh.num_face_vertices.size(); i++) {
      assert(mesh.num_face_vertices[i] == 3);
      verts.emplace_back();
      for (size_t j = 0; j < mesh.num_face_vertices[i]; j++) {
        tinyobj::index_t index = mesh.indices[idx++];

        // 1. Позиция вершины
        int vid = index.vertex_index;
        verts.back().v[j] = glm::vec3(attrib.vertices[vid * 3 + 0],
                                      attrib.vertices[vid * 3 + 1],
                                      attrib.vertices[vid * 3 + 2]);

        // 2. Нормаль вершины (если есть в файле)
        int nid = index.normal_index;
        if (nid >= 0) {
          verts.back().n[j] = glm::normalize(glm::vec3(
              attrib.normals[nid * 3 + 0], attrib.normals[nid * 3 + 1],
              attrib.normals[nid * 3 + 2]));
        }

        // 3. Текстурные координаты (если есть в файле)
        int tid = index.texcoord_index;
        if (tid >= 0) {
          verts.back().uv[j] = glm::vec2(attrib.texcoords[tid * 2 + 0],
                                         attrib.texcoords[tid * 2 + 1]);
        }
      }
    }
  }
  return verts;
}
