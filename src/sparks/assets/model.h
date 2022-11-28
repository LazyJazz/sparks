#pragma once
#include "sparks/assets/vertex.h"
#include "vector"

namespace sparks {
class Model {
 public:
  Model(const std::vector<Vertex> &vertices,
        const std::vector<uint32_t> &indices);
  static Model Cube(const glm::vec3 &center, const glm::vec3 &size);
  static Model Sphere(const glm::vec3 &center, float radius);

 private:
  std::vector<Vertex> vertices_;
  std::vector<uint32_t> indices_;
};
}  // namespace sparks
