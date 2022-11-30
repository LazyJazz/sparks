
struct Material {
  vec3 albedo_color;
  int albedo_texture_id;
  uint material_type;
};

#define MATERIAL_TYPE_LAMBERTIAN 0
#define MATERIAL_TYPE_PRINCIPLED 1
