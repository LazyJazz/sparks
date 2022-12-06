#include "sparks/assets/entity.h"

namespace sparks {

const Model *Entity::GetModel() const {
  return model_.get();
}

glm::mat4 &Entity::GetTransformMatrix() {
  return transform_;
}

const glm::mat4 &Entity::GetTransformMatrix() const {
  return transform_;
}

Material &Entity::GetMaterial() {
  return material_;
}

const Material &Entity::GetMaterial() const {
  return material_;
}

const std::string &Entity::GetName() const {
  return name_;
}

}  // namespace sparks
