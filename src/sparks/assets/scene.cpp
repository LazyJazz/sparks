#include "sparks/assets/scene.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace sparks {

Scene::Scene() {
  textures_.push_back(Texture(1, 1, {0.6f, 0.7f, 0.8f}, SAMPLE_TYPE_LINEAR));
  Texture envmap(SAMPLE_TYPE_LINEAR);
  Texture::Load("../../textures/envmap_parking_lot_5120.hdr", envmap);
  envmap_id_ = AddTexture(envmap);
  AddEntity(Mesh::Sphere(glm::vec3{0.0f, 0.5f, 0.0f}, 0.5f), Material{},
            glm::mat4{1.0f});
  AddEntity(Mesh({{{-1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                  {{-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                  {{1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                  {{1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}},
                 {0, 1, 2, 1, 2, 3}),
            Material{}, glm::mat4{1.0f});
  camera_to_world_ = glm::inverse(glm::lookAt(glm::vec3{2.0f, 1.0f, 3.0f},
                                              glm::vec3{0.0f, 0.0f, 0.0f},
                                              glm::vec3{0.0f, 1.0f, 0.0f}));
}

int Scene::AddTexture(const Texture &texture) {
  textures_.push_back(texture);
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

void Scene::SetCamera(const Camera &camera) {
  camera_ = camera;
}

const glm::mat4 &Scene::GetCameraToWorld() const {
  return camera_to_world_;
}

}  // namespace sparks
