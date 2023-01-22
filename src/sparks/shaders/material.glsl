#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

struct Material {
  vec3 base_color;
  int base_color_texture_id;

  vec3 subsurface_color;
  float subsurface;

  vec3 subsurface_radius;
  float metallic;

  float specular;
  float specular_tint;
  float roughness;
  float anisotropic;

  float anisotropic_rotation;
  float sheen;
  float sheen_tint;
  float clearcoat;

  float clearcoat_roughness;
  float ior;
  float transmission;
  float transmission_roughness;

  vec3 emission;
  float emission_strength;

  float alpha;
  uint material_type;
  int normal_map_id;
  float normal_map_intensity;

  int metallic_texture_id;
  int roughness_texture_id;
  int reserve[2];
};

#define MATERIAL_TYPE_LAMBERTIAN 0
#define MATERIAL_TYPE_SPECULAR 1
#define MATERIAL_TYPE_TRANSMISSIVE 2
#define MATERIAL_TYPE_PRINCIPLED 3
#define MATERIAL_TYPE_EMISSION 4

#endif
