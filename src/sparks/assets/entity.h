#pragma once
#include "memory"
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
  template <class ModelType>
  Entity(const ModelType &model,
         const Material &material,
         const glm::mat4 &transform = glm::mat4{1.0f}) {
    model_ = std::make_unique<ModelType>(model);
    material_ = material;
    transform_ = transform;
  }
  [[nodiscard]] const Model *GetModel() const;
  [[nodiscard]] glm::mat4 &GetTransformMatrix();
  [[nodiscard]] const glm::mat4 &GetTransformMatrix() const;
  [[nodiscard]] Material &GetMaterial();
  [[nodiscard]] const Material &GetMaterial() const;

 private:
  std::unique_ptr<Model> model_;
  Material material_{};
  glm::mat4 transform_{1.0f};
};
}  // namespace sparks
