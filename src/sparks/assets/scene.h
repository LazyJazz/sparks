#pragma once
#include "memory"
#include "sparks/assets/camera.h"
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

  void SetCamera(const Camera &camera);
  Camera &GetCamera();

  [[nodiscard]] const glm::mat4 &GetCameraToWorld() const;

  void Clear();

 private:
  std::vector<Texture> textures_;
  std::vector<Entity> entities_;
  glm::mat4 camera_to_world_{1.0f};
  Camera camera_{};
};
}  // namespace sparks
