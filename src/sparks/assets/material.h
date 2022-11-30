#pragma once
#include "cstdint"
#include "glm/glm.hpp"

namespace sparks {

enum MaterialType : uint32_t {
  MATERIAL_TYPE_LAMBERTIAN = 0,
  MATERIAL_TYPE_PRINCIPLED = 1
};

struct Material {
  glm::vec3 albedo_color{0.8f};
  int albedo_texture_id{0};
  MaterialType material_type{MATERIAL_TYPE_LAMBERTIAN};
  int reserve[3];
};
}  // namespace sparks
