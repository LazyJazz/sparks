#pragma once
#include "glm/glm.hpp"

namespace sparks {
struct GlobalUniformObject {
  glm::mat4 projection{1.0f};
  glm::mat4 camera{1.0f};
  int envmap_id{0};
  float envmap_offset{0.0f};
};
}  // namespace sparks
