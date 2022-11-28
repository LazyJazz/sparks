#include "sparks/assets/scene.h"

namespace sparks {

Scene::Scene() {
  textures_.push_back(Texture(1, 1, {0.6f, 0.7f, 0.8f}));
}

int Scene::PushTexture(const Texture &texture) {
  textures_.push_back(texture);
  return int(textures_.size() - 1);
}

const std::vector<Texture> &Scene::GetTextures() const {
  return textures_;
}

int Scene::GetTextureCount() {
  return int(textures_.size());
}

}  // namespace sparks
