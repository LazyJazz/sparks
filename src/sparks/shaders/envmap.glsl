#ifndef ENVMAP_GLSL
#define ENVMAP_GLSL
#include "sample_texture.glsl"

vec2 DirectionToEnvmapCoord(vec3 direction) {
  float x = global_uniform_object.envmap_offset;
  float y = acos(direction.y) * INV_PI;
  if (length(direction.xy) > 1e-4) {
    x += atan(direction.x, -direction.z);
  }
  x *= INV_PI * 0.5;
  return vec2(x, y);
}

vec3 EnvmapCoordToDirection(vec2 envmap_tex_coord) {
  envmap_tex_coord.x *= 2.0 * PI;
  envmap_tex_coord.x -= global_uniform_object.envmap_offset;
  envmap_tex_coord.y *= PI;
  float sin_y = sin(envmap_tex_coord.y);
  return vec3(sin(envmap_tex_coord.x) * sin_y, cos(envmap_tex_coord.y),
              -cos(envmap_tex_coord.x) * sin_y);
}

vec3 SampleEnvmapTexCoord(vec2 tex_coord) {
  return SampleTexture(global_uniform_object.envmap_id, tex_coord).xyz;
}

vec3 SampleEnvmap(vec3 direction) {
  return SampleEnvmapTexCoord(DirectionToEnvmapCoord(direction));
}

#endif
