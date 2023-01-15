#pragma once

namespace sparks {
struct ObjectSamplerInfo {
  glm::mat4 local_to_world{1.0f};
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
