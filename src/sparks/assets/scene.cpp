#include "sparks/assets/scene.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include "sparks/assets/accelerated_mesh.h"

namespace sparks {

Scene::Scene() {
  AddTexture(Texture(1, 1, glm::vec4{1.0f}, SAMPLE_TYPE_LINEAR), "Pure White");
  AddTexture(Texture(1, 1, glm::vec4{0.0f}, SAMPLE_TYPE_LINEAR), "Pure Black");
  Texture envmap;
  Texture::Load(u8"../../textures/envmap_clouds_4k.hdr", envmap);
  envmap.SetSampleType(SAMPLE_TYPE_LINEAR);
  envmap_id_ = AddTexture(envmap, "Clouds");
  AddEntity(
      AcceleratedMesh({{{-1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                       {{-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                       {{1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                       {{1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}},
                      {0, 1, 2, 2, 1, 3}),
      Material{}, glm::mat4{1.0f});
  SetCameraToWorld(glm::inverse(glm::lookAt(glm::vec3{2.0f, 1.0f, 3.0f},
                                            glm::vec3{0.0f, 0.0f, 0.0f},
                                            glm::vec3{0.0f, 1.0f, 0.0f})));

  Texture texture;
  Texture::Load("../../textures/earth.jpg", texture);
  //  AddEntity(Mesh::Sphere(glm::vec3{0.0f, 0.0f, 0.0f}, 0.5f),
  //              Material{glm::vec3{1.0f}, AddTexture(texture)},
  //              glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, 0.5f, 0.0f}));
  AddEntity(AcceleratedMesh(Mesh::Sphere(glm::vec3{0.0f, 0.0f, 0.0f}, 0.5f)),
            Material{glm::vec3{1.0f}, AddTexture(texture, "Earth")},
            glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, 0.5f, 0.0f}));
}

int Scene::AddTexture(const Texture &texture, const std::string &name) {
  textures_.push_back(texture);
  texture_names_.push_back(name);
  return int(textures_.size() - 1);
}

const std::vector<Texture> &Scene::GetTextures() const {
  return textures_;
}

int Scene::GetTextureCount() const {
  return int(textures_.size());
}

void Scene::Clear() {
  textures_.clear();
  entities_.clear();
  camera_ = Camera{};
}

Entity &Scene::GetEntity(int entity_index) {
  return entities_[entity_index];
}

const Entity &Scene::GetEntity(int entity_index) const {
  return entities_[entity_index];
}

std::vector<Entity> &Scene::GetEntities() {
  return entities_;
}

const std::vector<Entity> &Scene::GetEntities() const {
  return entities_;
}

int Scene::GetEntityCount() const {
  return int(entities_.size());
}

Camera &Scene::GetCamera() {
  return camera_;
}

const Camera &Scene::GetCamera() const {
  return camera_;
}

void Scene::SetCamera(const Camera &camera) {
  camera_ = camera;
}

glm::mat4 Scene::GetCameraToWorld() const {
  return glm::translate(glm::mat4{1.0f}, camera_position_) *
         ComposeRotation(camera_pitch_yaw_roll_) *
         glm::scale(glm::mat4{1.0f}, glm::vec3{1.0f, 1.0f, 1.0f});
}

void Scene::SetCameraToWorld(const glm::mat4 &camera_to_world) {
  camera_pitch_yaw_roll_ = DecomposeRotation(
      camera_to_world *
      glm::scale(glm::mat4{1.0f}, glm::vec3{1.0f, 1.0f, 1.0f}));
  camera_position_ = camera_to_world[3];
}

int &Scene::GetEnvmapId() {
  return envmap_id_;
}

const int &Scene::GetEnvmapId() const {
  return envmap_id_;
}

float &Scene::GetEnvmapOffset() {
  return envmap_offset_;
}

const float &Scene::GetEnvmapOffset() const {
  return envmap_offset_;
}

glm::vec3 &Scene::GetCameraPosition() {
  return camera_position_;
}
const glm::vec3 &Scene::GetCameraPosition() const {
  return camera_position_;
}
glm::vec3 &Scene::GetCameraPitchYawRoll() {
  return camera_pitch_yaw_roll_;
}
const glm::vec3 &Scene::GetCameraPitchYawRoll() const {
  return camera_pitch_yaw_roll_;
}

void Scene::UpdateEnvmapConfiguration() {
  const auto &scene = *this;
  auto envmap_id = scene.GetEnvmapId();
  auto &envmap_texture = scene.GetTextures()[envmap_id];
  auto buffer = envmap_texture.GetBuffer();

  envmap_minor_color_ = glm::vec3{0.0f};
  envmap_major_color_ = glm::vec3{0.0f};
  envmap_cdf_.resize(envmap_texture.GetWidth() * envmap_texture.GetHeight());

  std::vector<float> sample_scale_(envmap_texture.GetHeight() + 1);
  auto inv_width = 1.0f / float(envmap_texture.GetWidth());
  auto inv_height = 1.0f / float(envmap_texture.GetHeight());
  for (int i = 0; i <= envmap_texture.GetHeight(); i++) {
    float x = float(i) * glm::pi<float>() * inv_height;
    sample_scale_[i] = (-std::cos(x) + 1.0f) * 0.5f;
  }

  auto width_height = envmap_texture.GetWidth() * envmap_texture.GetHeight();
  float total_weight = 0.0f;
  float major_strength = -1.0f;
  for (int y = 0; y < envmap_texture.GetHeight(); y++) {
    auto scale = sample_scale_[y + 1] - sample_scale_[y];

    auto theta = (float(y) + 0.5f) * inv_height * glm::pi<float>();
    auto sin_theta = std::sin(theta);
    auto cos_theta = std::cos(theta);

    for (int x = 0; x < envmap_texture.GetWidth(); x++) {
      auto phi = (float(x) + 0.5f) * inv_width * glm::pi<float>() * 2.0f;
      auto sin_phi = std::sin(phi);
      auto cos_phi = std::cos(phi);

      auto i = y * envmap_texture.GetWidth() + x;
      auto color = glm::vec3{buffer[i]};
      auto minor_color = glm::clamp(color, 0.0f, 1.0f);
      auto major_color = color - minor_color;
      envmap_major_color_ += major_color * (scale * inv_width);
      envmap_minor_color_ += minor_color * (scale * inv_width);
      color *= scale;

      auto strength = std::max(color.x, std::max(color.y, color.z));
      if (strength > major_strength) {
        envmap_light_direction_ = {sin_theta * sin_phi, cos_theta,
                                   -sin_theta * cos_phi};
        major_strength = strength;
      }

      total_weight += strength * scale;
      envmap_cdf_[i] = total_weight;
    }
  }

  auto inv_total_weight = 1.0f / total_weight;
  for (auto &v : envmap_cdf_) {
    v *= inv_total_weight;
  }
}
glm::vec3 Scene::GetEnvmapLightDirection() const {
  float sin_offset = std::sin(envmap_offset_);
  float cos_offset = std::cos(envmap_offset_);
  return {cos_offset * envmap_light_direction_.x +
              sin_offset * envmap_light_direction_.z,
          envmap_light_direction_.y,
          -sin_offset * envmap_light_direction_.x +
              cos_offset * envmap_light_direction_.z};
}
const glm::vec3 &Scene::GetEnvmapMinorColor() const {
  return envmap_minor_color_;
}
const glm::vec3 &Scene::GetEnvmapMajorColor() const {
  return envmap_major_color_;
}
const std::vector<float> &Scene::GetEnvmapCdf() const {
  return envmap_cdf_;
}

float Scene::TraceRay(const glm::vec3 &origin,
                      const glm::vec3 &direction,
                      float t_min,
                      float t_max,
                      HitRecord *hit_record) const {
  float result = -1.0f;
  HitRecord local_hit_record;
  float local_result;
  for (int entity_id = 0; entity_id < entities_.size(); entity_id++) {
    auto &entity = entities_[entity_id];
    auto &transform = entity.GetTransformMatrix();
    auto inv_transform = glm::inverse(transform);
    auto transformed_direction =
        glm::vec3{inv_transform * glm::vec4{direction, 0.0f}};
    auto transformed_direction_length = glm::length(transformed_direction);
    if (transformed_direction_length < 1e-6) {
      continue;
    }
    local_result = entity.GetModel()->TraceRay(
        inv_transform * glm::vec4{origin, 1.0f},
        transformed_direction / transformed_direction_length, t_min,
        hit_record ? &local_hit_record : nullptr);
    local_result /= transformed_direction_length;
    if (local_result > t_min && local_result < t_max &&
        (result < 0.0f || local_result < result)) {
      result = local_result;
      if (hit_record) {
        local_hit_record.position =
            transform * glm::vec4{local_hit_record.position, 1.0f};
        local_hit_record.normal = glm::transpose(inv_transform) *
                                  glm::vec4{local_hit_record.normal, 0.0f};
        local_hit_record.tangent =
            transform * glm::vec4{local_hit_record.tangent, 0.0f};
        local_hit_record.geometry_normal =
            glm::transpose(inv_transform) *
            glm::vec4{local_hit_record.geometry_normal, 0.0f};
        *hit_record = local_hit_record;
        hit_record->hit_entity_id = entity_id;
      }
    }
  }
  if (hit_record) {
    hit_record->geometry_normal = glm::normalize(hit_record->geometry_normal);
    hit_record->normal = glm::normalize(hit_record->normal);
    hit_record->tangent = glm::normalize(hit_record->tangent);
  }
  return result;
}

glm::vec4 Scene::SampleEnvmap(const glm::vec3 &direction) const {
  float x = envmap_offset_;
  float y = acos(direction.y) * INV_PI;
  if (glm::length(glm::vec2{direction.x, direction.y}) > 1e-4) {
    x += glm::atan(direction.x, -direction.z);
  }
  x *= INV_PI * 0.5;
  return textures_[envmap_id_].Sample(glm::vec2{x, y});
}

const Texture &Scene::GetTexture(int texture_id) const {
  return textures_[texture_id];
}

std::vector<const char *> Scene::GetTextureNameList() const {
  std::vector<const char *> result;
  result.reserve(texture_names_.size());
  for (const auto &texture_name : texture_names_) {
    result.push_back(texture_name.data());
  }
  return result;
}

bool Scene::TextureCombo(const char *label, int *current_item) const {
  return ImGui::Combo(label, current_item, GetTextureNameList().data(),
                      textures_.size());
}

int Scene::LoadTexture(const std::string &file_path) {
  Texture texture;
  if (Texture::Load(file_path, texture)) {
    return AddTexture(texture, file_path);
  } else {
    return 0;
  }
}

}  // namespace sparks
