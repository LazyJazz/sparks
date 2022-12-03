
struct GlobalUniformObject {
  mat4 projection;
  mat4 camera;
  vec3 envmap_light_direction;
  int envmap_id;
  vec3 envmap_minor_color;
  float envmap_offset;
  vec3 envmap_major_color;
  int hover_id;
  int selected_id;
};

struct EntityUniformObject {
  mat4 model;
};
