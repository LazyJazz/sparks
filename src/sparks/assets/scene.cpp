#include "sparks/assets/scene.h"

namespace sparks {

Scene::Scene() {
  textures_.push_back(Texture(1, 1, {0.6f, 0.7f, 0.8f}));
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

}  // namespace sparks
