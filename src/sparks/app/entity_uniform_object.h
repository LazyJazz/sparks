#pragma once
#include "glm/glm.hpp"

namespace sparks {
struct EntityUniformObject {
  glm::mat4 object_to_world{1.0f};
  float cdf{1.0f};
  float pdf{0.0f};
  int primitive_offset{0};
  int num_primitives{1};
  float power{0.0f};
  float area{0.0f};
  float sample_density{};
  float reserve{};
};
}  // namespace sparks
