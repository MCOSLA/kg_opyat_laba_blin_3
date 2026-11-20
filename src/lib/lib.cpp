#include "lib.hpp"

#include <stb_image.h>
#include <stb_image_write.h>
#include <tiny_obj_loader.h>

#include <algorithm>
#include <cmath>
#include <glm/ext.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <limits>

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

// ---------- Текстура ----------
bool Texture::load(const char* path) {
  int n;
  unsigned char* d = stbi_load(path, &w, &h, &n, 3);  // принудительно 3 канала
  if (!d) {
    std::cerr << "Failed to load texture: " << path << std::endl;
    return false;
  }
  data.resize(size_t(w) * h);
  for (size_t i = 0; i < data.size(); i++)
    data[i] = glm::vec3(d[3 * i], d[3 * i + 1], d[3 * i + 2]) / 255.0f;
  stbi_image_free(d);
  return true;
}

glm::vec3 Texture::sample(glm::vec2 uv) const {
  if (data.empty()) return glm::vec3(1.0f);
  float u = uv.x - std::floor(uv.x);  // повтор текстуры
  float v = uv.y - std::floor(uv.y);
  int x = std::min(int(u * w), w - 1);
  int y = std::min(int((1.0f - v) * h), h - 1);  // в obj v идёт снизу вверх
  return data[size_t(y) * w + x];
}

// ---------- Шейдеры ----------
VertexOut vertexShader(const Uniforms& u, const glm::vec3& pos,
                       const glm::vec3& n, const glm::vec2& uv) {
  VertexOut o;
  glm::vec4 world = u.model * glm::vec4(pos, 1.0f);
  o.worldPos = glm::vec3(world);
  o.clip = u.proj * u.view * world;
  o.normal = glm::mat3(glm::transpose(glm::inverse(u.model))) * n;
  o.uv = uv;
  return o;
}

glm::vec3 fragmentShader(const Uniforms& u, const VertexOut& in) {
  glm::vec3 albedo = u.texture ? u.texture->sample(in.uv) : glm::vec3(0.8f);

  glm::vec3 N = glm::normalize(in.normal);
  glm::vec3 L = glm::normalize(u.lightPos - in.worldPos);
  glm::vec3 V = glm::normalize(u.cameraPos - in.worldPos);
  glm::vec3 R = glm::reflect(-L, N);

  float diff = std::max(glm::dot(N, L), 0.0f);
  float spec = 0.0f;
  if (diff > 0.0f) spec = std::pow(std::max(glm::dot(R, V), 0.0f), u.shininess);

  glm::vec3 col = albedo * (u.ambient + diff) * u.lightColor +
                  u.specStrength * spec * u.lightColor;
  return glm::clamp(col, 0.0f, 1.0f);
}

// ---------- Растеризатор ----------
static float edgeFn(const glm::vec2& a, const glm::vec2& b,
                    const glm::vec2& p) {
  return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

void drawTriangle(const Uniforms& u, const VertexOut& a, const VertexOut& b,
                  const VertexOut& c, Image<glm::vec3>& color,
                  Image<float>& depth) {
  const VertexOut* v[3] = {&a, &b, &c};
  glm::vec2 scr[3];
  float z[3], invW[3];

  for (int i = 0; i < 3; i++) {
    float w = v[i]->clip.w;
    if (w <= 1e-6f) return;  // вершина за камерой (упрощённо пропускаем)
    invW[i] = 1.0f / w;
    glm::vec3 ndc = glm::vec3(v[i]->clip) * invW[i];  // деление на w
    scr[i] = glm::vec2((ndc.x * 0.5f + 0.5f) * (color.width() - 1),
                       (ndc.y * 0.5f + 0.5f) * (color.height() - 1));
    z[i] = ndc.z;
  }

  float area = edgeFn(scr[0], scr[1], scr[2]);
  if (std::abs(area) < 1e-8f) return;
  if (u.cullBack && area < 0.0f) return;  // задняя грань

  int minX =
      std::max(0, (int)std::floor(std::min({scr[0].x, scr[1].x, scr[2].x})));
  int maxX = std::min(color.width() - 1,
                      (int)std::ceil(std::max({scr[0].x, scr[1].x, scr[2].x})));
  int minY =
      std::max(0, (int)std::floor(std::min({scr[0].y, scr[1].y, scr[2].y})));
  int maxY = std::min(color.height() - 1,
                      (int)std::ceil(std::max({scr[0].y, scr[1].y, scr[2].y})));

  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      glm::vec2 p = glm::vec2(float(x), float(y));
      // барицентрические координаты
      float w0 = edgeFn(scr[1], scr[2], p) / area;
      float w1 = edgeFn(scr[2], scr[0], p) / area;
      float w2 = edgeFn(scr[0], scr[1], p) / area;
      if (w0 < 0 || w1 < 0 || w2 < 0) continue;  // пиксель вне треугольника

      float zi = w0 * z[0] + w1 * z[1] + w2 * z[2];
      if (zi < -1.0f || zi > 1.0f) continue;
      if (zi >= depth.at(x, y)) continue;  // z-тест
      depth.at(x, y) = zi;

      // перспективно-корректная интерполяция
      float p0 = w0 * invW[0], p1 = w1 * invW[1], p2 = w2 * invW[2];
      float s = p0 + p1 + p2;
      p0 /= s;
      p1 /= s;
      p2 /= s;

      VertexOut f;
      f.worldPos = p0 * a.worldPos + p1 * b.worldPos + p2 * c.worldPos;
      f.normal = p0 * a.normal + p1 * b.normal + p2 * c.normal;
      f.uv = p0 * a.uv + p1 * b.uv + p2 * c.uv;
      f.clip = glm::vec4(0.0f);

      color.at(x, y) = fragmentShader(u, f);
    }
  }
}

void renderMesh(const std::vector<Triangle>& tris, const Uniforms& u,
                Image<glm::vec3>& color, Image<float>& depth) {
  for (const auto& t : tris) {
    glm::vec3 faceN =
        glm::normalize(glm::cross(t.v[1] - t.v[0], t.v[2] - t.v[0]));
    VertexOut out[3];
    for (int i = 0; i < 3; i++) {
      glm::vec3 n = t.n[i];
      if (glm::dot(n, n) < 1e-8f) n = faceN;  // в obj нет нормалей
      out[i] = vertexShader(u, t.v[i], n, t.uv[i]);
    }
    drawTriangle(u, out[0], out[1], out[2], color, depth);
  }
}
