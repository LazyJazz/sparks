#pragma once
#include "memory"
#include "sparks/assets/camera.h"
#include "sparks/assets/entity.h"
#include "sparks/assets/material.h"
#include "sparks/assets/mesh.h"
#include "sparks/assets/texture.h"
#include "sparks/assets/util.h"
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

  [[nodiscard]] Entity &GetEntity(int entity_index);
  [[nodiscard]] const Entity &GetEntity(int entity_index) const;
  [[nodiscard]] std::vector<Entity> &GetEntities();
  [[nodiscard]] const std::vector<Entity> &GetEntities() const;
  [[nodiscard]] int GetEntityCount() const;

  void SetCamera(const Camera &camera);
  Camera &GetCamera();
  [[nodiscard]] glm::mat4 GetCameraToWorld() const;
  void SetCameraToWorld(const glm::mat4 &camera_to_world);

  int &GetEnvmapId();
  [[nodiscard]] const int &GetEnvmapId() const;
  float &GetEnvmapOffset();
  [[nodiscard]] const float &GetEnvmapOffset() const;
  glm::vec3 &GetCameraPosition();
  [[nodiscard]] const glm::vec3 &GetCameraPosition() const;
  glm::vec3 &GetCameraPitchYawRoll();
  [[nodiscard]] const glm::vec3 &GetCameraPitchYawRoll() const;

  void Clear();

 private:
  std::vector<Texture> textures_;
  std::vector<Entity> entities_;
  int envmap_id_{0};
  float envmap_offset_{0.0f};
  glm::vec3 camera_position_{0.0f};
  glm::vec3 camera_pitch_yaw_roll_{0.0f, 0.0f, 0.0f};
  Camera camera_{};
};
}  // namespace sparks
