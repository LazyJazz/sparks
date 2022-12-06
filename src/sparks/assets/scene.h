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
  int AddTexture(const Texture &texture,
                 const std::string &name = "Unnamed Texture");
  [[nodiscard]] const std::vector<Texture> &GetTextures() const;
  [[nodiscard]] const Texture &GetTexture(int texture_id) const;
  [[nodiscard]] int GetTextureCount() const;
  [[nodiscard]] std::vector<const char *> GetTextureNameList() const;

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
  [[nodiscard]] std::vector<const char *> GetEntityNameList() const;

  void SetCamera(const Camera &camera);
  Camera &GetCamera();
  [[nodiscard]] const Camera &GetCamera() const;
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
  void UpdateEnvmapConfiguration();

  [[nodiscard]] glm::vec3 GetEnvmapLightDirection() const;
  [[nodiscard]] const glm::vec3 &GetEnvmapMinorColor() const;
  [[nodiscard]] const glm::vec3 &GetEnvmapMajorColor() const;
  [[nodiscard]] const std::vector<float> &GetEnvmapCdf() const;
  ;
  [[nodiscard]] glm::vec4 SampleEnvmap(const glm::vec3 &direction) const;

  float TraceRay(const glm::vec3 &origin,
                 const glm::vec3 &direction,
                 float t_min,
                 float t_max,
                 HitRecord *hit_record) const;

  bool TextureCombo(const char *label, int *current_item) const;
  bool EntityCombo(const char *label, int *current_item) const;
  int LoadTexture(const std::string &file_path);
  int LoadObjMesh(const std::string &file_path);

 private:
  std::vector<Texture> textures_;
  std::vector<std::string> texture_names_;

  std::vector<Entity> entities_;

  int envmap_id_{0};
  float envmap_offset_{0.0f};
  std::vector<float> envmap_cdf_;
  glm::vec3 envmap_light_direction_{0.0f, 1.0f, 0.0f};
  glm::vec3 envmap_major_color_{0.5f};
  glm::vec3 envmap_minor_color_{0.3f};

  glm::vec3 camera_position_{0.0f};
  glm::vec3 camera_pitch_yaw_roll_{0.0f, 0.0f, 0.0f};
  Camera camera_{};
};
}  // namespace sparks
