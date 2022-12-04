#pragma once
#include "glm/glm.hpp"

namespace sparks {
struct HitRecord {
  int hit_entity_id{-1};
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec3 geometry_normal{};
  glm::vec3 tangent{};
  glm::vec2 tex_coord{};
  bool front_face{};
};
}  // namespace sparks
