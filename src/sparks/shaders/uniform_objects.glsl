#ifndef UNIFORM_OBJECTS_GLSL
#define UNIFORM_OBJECTS_GLSL
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
  int accumulated_sample;
  int num_samples;
  int num_bounces;

  int num_objects;
  float fov;
  float aperture;
  float focal_length;

  float clamp;
  float gamma;
  float aspect;
  float total_power;

  float total_envmap_power;
  bool enable_mis;
  int output_selection;
  int reserve;
};

struct EntityUniformObject {
  mat4 object_to_world;
  float cdf;
  float pdf;
  int primitive_offset;
  int num_primitives;
  float power;
  float area;
  float sample_density;
  float reserve;
};

struct ObjectInfo {
  uint vertex_offset;
  uint index_offset;
};

struct ObjectSamplerInfo {
  mat4 object_to_world;
  float cdf;
  float pdf;
  int primitive_offset;
  int num_primitives;
  float power;
  float area;
  float sample_density;
  float reserve;
};

#endif
