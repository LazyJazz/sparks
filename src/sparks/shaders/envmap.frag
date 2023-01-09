#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable
#include "constants.glsl"
#include "material.glsl"
#include "uniform_objects.glsl"

layout(binding = 0) readonly uniform global_uniform {
  GlobalUniformObject global_uniform_object;
};
layout(binding = 1) uniform sampler2D[] texture_samplers;

layout(location = 0) in vec2 tex_coord;
layout(location = 0) out vec4 color_out;

vec3 SampleEnvmap(vec3 direction) {
  float x = global_uniform_object.envmap_offset;
  float y = acos(direction.y) * INV_PI;
  if (length(vec2(direction.x, direction.y)) > 1e-4) {
    x += atan(direction.x, -direction.z);
  }
  x *= INV_PI * 0.5;
  return texture(texture_samplers[global_uniform_object.envmap_id], vec2(x, y))
      .xyz;
}

void main() {
  mat4 camera_to_world = inverse(global_uniform_object.camera);
  mat4 screen_to_camera = inverse(global_uniform_object.projection);
  vec3 origin = vec3(camera_to_world * vec4(0, 0, 0, 1));
  vec3 target = vec3(screen_to_camera * vec4(tex_coord, 1.0, 1.0));
  target = normalize(target);
  target *= -global_uniform_object.focal_length / target.z;
  target = vec3(camera_to_world * vec4(target, 1.0));
  vec3 direction = normalize(target - origin);
  color_out = vec4(SampleEnvmap(direction), 1.0);
}
