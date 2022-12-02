#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable
#include "constants.glsl"
#include "material.glsl"
#include "uniform_objects.glsl"

layout(binding = 0) readonly uniform global_uniform_object {
  GlobalUniformObject global_object;
};
layout(binding = 1) uniform sampler2D[] texture_samplers;

layout(location = 0) in vec2 tex_coord;
layout(location = 0) out vec4 color_out;

void main() {
  vec3 direction =
      vec3(inverse(global_object.projection * global_object.camera) *
           vec4(tex_coord * 1000.0, 1000.0, 1000.0));
  direction = normalize(direction);
  float x = global_object.envmap_offset;
  float y = acos(direction.y) * INV_PI;
  if (length(direction.xy) > 1e-4) {
    x += atan(direction.x, -direction.z);
  }
  x *= INV_PI * 0.5;
  color_out = texture(texture_samplers[nonuniformEXT(global_object.envmap_id)],
                      vec2(x, y));
}
