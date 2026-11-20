#include <iostream>
#include <lib.hpp>
#include <limits>

int main(int argc, char** argv) {
  if (argc <= 1) {
    std::cerr << "Usage: model <path.obj> [texture.png|tga]\n";
    return 1;
  }
  const int W = 1024, H = 1024;
  Image<glm::vec3> color(W, H, glm::vec3(0.0f));
  Image<float> depth(W, H, std::numeric_limits<float>::infinity());

  auto tris = loadObj(argv[1]);
  if (tris.empty()) return 1;

  Texture tex;
  bool hasTex = argc > 2 && tex.load(argv[2]);

  Camera cam;
  cam.position = glm::vec3(0.0f, 0.0f, 3.5f);
  cam.aspect = float(W) / float(H);

  Uniforms u;
  u.model = glm::mat4(1.0f);
  u.view = cam.view();
  u.proj = cam.projection();
  u.cameraPos = cam.position;
  u.texture = hasTex ? &tex : nullptr;

  renderMesh(tris, u, color, depth);
  writeImg(color, "model.png");
  return 0;
}
