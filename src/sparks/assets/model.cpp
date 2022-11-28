#include "sparks/assets/model.h"

namespace sparks {

Model::Model(const std::vector<Vertex> &vertices,
             const std::vector<uint32_t> &indices) {
  vertices_ = vertices;
  indices_ = indices;
}

Model Model::Cube(const glm::vec3 &center, const glm::vec3 &size) {
  return {{}, {}};
}

Model Model::Sphere(const glm::vec3 &center, float radius) {
  return {{}, {}};
}

}  // namespace sparks
