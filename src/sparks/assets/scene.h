#pragma once
#include "memory"
#include "sparks/assets/mesh.h"
#include "sparks/assets/texture.h"
#include "vector"

namespace sparks {
class Scene {
 public:
  Scene();
  int PushTexture(const Texture &texture);
  [[nodiscard]] const std::vector<Texture> &GetTextures() const;
  int GetTextureCount();

 private:
  std::vector<Texture> textures_;
};
}  // namespace sparks
