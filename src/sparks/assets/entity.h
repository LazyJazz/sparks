#pragma once
#include "sparks/assets/material.h"
#include "sparks/assets/mesh.h"
#include "sparks/assets/model.h"

namespace sparks {
class Entity {
 public:
  Entity(std::unique_ptr<Model> &&model,
         const Material &material,
         const glm::mat4 &transform = glm::mat4{1.0f});
  Entity(const Mesh &mesh,
         const Material &material,
         const glm::mat4 &transform = glm::mat4{1.0f});
  [[nodiscard]] const Model *GetModel() const;
  [[nodiscard]] const glm::mat4 &GetTransformMatrix() const;
  [[nodiscard]] const Material &GetMaterial() const;

 private:
  std::unique_ptr<Model> model_;
  Material material_{};
  glm::mat4 transform_{1.0f};
};
}  // namespace sparks
