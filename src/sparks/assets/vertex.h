#pragma once
#include "glm/glm.hpp"

namespace sparks {
struct Vertex {
  glm::vec3 position{};
  glm::vec3 normal{0.0f, 0.0f, 1.0f};
  glm::vec3 tangent{1.0f, 0.0f, 0.0f};
  glm::vec2 tex_coord{};

  Vertex() = default;

  Vertex(const glm::vec3 &position,
         const glm::vec3 &normal,
         const glm::vec2 &tex_coord) {
    this->position = position;
    this->normal = normal;
    this->tex_coord = tex_coord;
  }

  bool operator==(const Vertex &vertex) const {
    return position == vertex.position && normal == vertex.normal &&
           tangent == vertex.tangent && tex_coord == tex_coord;
  }
};
}  // namespace sparks
