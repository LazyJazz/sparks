#pragma once
#include "cstdint"
#include "glm/glm.hpp"
#include "sparks/assets/util.h"

namespace sparks {

enum MaterialType : int {
  MATERIAL_TYPE_LAMBERTIAN = 0,
  MATERIAL_TYPE_SPECULAR = 1,
  MATERIAL_TYPE_TRANSMISSIVE = 2,
  MATERIAL_TYPE_PRINCIPLED = 3,
  MATERIAL_TYPE_EMISSION = 4
};

class Scene;

struct Material {
  glm::vec3 base_color{0.8f};
  int base_color_texture_id{0};

  glm::vec3 subsurface_color{};
  float subsurface{0.0f};

  glm::vec3 subsurface_radius{1.0f, 0.2f, 0.1f};
  float metallic{0.0f};

  float specular{0.5f};
  float specular_tint{0.0f};
  float roughness{0.5f};
  float anisotropic{0.0f};

  float anisotropic_rotation{0.0f};
  float sheen{0.0f};
  float sheen_tint{0.0f};
  float clearcoat{0.0f};

  float clearcoat_roughness{0.03f};
  float ior{1.45f};
  float transmission{0.0f};
  float transmission_roughness{0.0f};

  glm::vec3 emission{1.0f};
  float emission_strength{0.0f};

  float alpha{1.0f};
  MaterialType material_type{MATERIAL_TYPE_LAMBERTIAN};
  int normal_map_id{-1};
  float normal_map_intensity{1.0f};
  Material() = default;
  explicit Material(const glm::vec3 &albedo);
  Material(Scene *scene, const tinyxml2::XMLElement *material_element);
};
}  // namespace sparks
