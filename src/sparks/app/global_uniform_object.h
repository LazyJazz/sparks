#pragma once
#include "glm/glm.hpp"

namespace sparks {
struct GlobalUniformObject {
  glm::mat4 projection{1.0f};
  glm::mat4 camera{1.0f};
  glm::vec3 envmap_light_direction{0.0f, 1.0f, 0.0f};
  int envmap_id{0};
  glm::vec3 envmap_minor_color{0.3f};
  float envmap_offset{0.0f};
  glm::vec3 envmap_major_color{0.5f};
  int hover_id{0};
  int selected_id{0};
  int accumulated_sample{0};
  int num_samples{0};
  int num_bounces{0};
  int num_objects{0};
  float fov{glm::radians(60.0f)};
  float aperture{0.0f};
  float focal_length{3.0f};
  float clamp{100.0f};
  float gamma{2.2f};
  float aspect{1.0f};
  float total_power{0.0f};
  float total_envmap_power{0.0f};
  int enable_mis{true};
};
}  // namespace sparks
