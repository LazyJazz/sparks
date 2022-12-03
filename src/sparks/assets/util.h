#pragma once
#include "glm/glm.hpp"

namespace sparks {
glm::vec3 DecomposeRotation(glm::mat3 R);

glm::mat4 ComposeRotation(glm::vec3 pitch_yaw_roll);
}  // namespace sparks
