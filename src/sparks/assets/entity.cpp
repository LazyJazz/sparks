#include "sparks/assets/entity.h"

namespace sparks {

Entity::Entity(std::unique_ptr<Model> &&model,
               const Material &material,
               const glm::mat4 &transform) {
  model_ = std::move(model);
  material_ = material;
  transform_ = transform;
}

Entity::Entity(const Mesh &mesh,
               const Material &material,
               const glm::mat4 &transform) {
  model_ = std::make_unique<Mesh>(mesh);
  material_ = material;
  transform_ = transform;
}

const Model *Entity::GetModel() const {
  return model_.get();
}

const glm::mat4 &Entity::GetTransformMatrix() const {
  return transform_;
}

const Material &Entity::GetMaterial() const {
  return material_;
}

}  // namespace sparks
