#pragma once
#include "memory"
#include "sparks/assets/entity.h"
#include "sparks/assets/material.h"
#include "sparks/assets/mesh.h"
#include "sparks/assets/texture.h"
#include "vector"

namespace sparks {
class Scene {
 public:
  Scene();
  int AddTexture(const Texture &texture);
  [[nodiscard]] const std::vector<Texture> &GetTextures() const;
  [[nodiscard]] int GetTextureCount() const;

  template <class... Args>
  int AddEntity(Args... args) {
    entities_.emplace_back(args...);
    return int(entities_.size() - 1);
  }
  [[nodiscard]] const std::vector<Entity> &GetEntities() const;
  [[nodiscard]] int GetEntityCount() const;

  void Clear();

 private:
  std::vector<Texture> textures_;
  std::vector<Entity> entities_;
};
}  // namespace sparks
