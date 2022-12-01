#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable
#include "defs.glsl"
#include "material.glsl"

layout(location = 0) in flat int instance_id;
layout(location = 1) in vec3 position;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec2 tex_coord;
layout(location = 0) out vec4 color_out;
layout(location = 1) out uvec4 instance_out;

layout(binding = 2) readonly buffer material_uniform_object {
  Material materials[];
};
layout(binding = 3) uniform sampler2D[] texture_samplers;

void main() {
  Material material = materials[instance_id];
  vec3 light = vec3(0.3);
  light += vec3(0.7) * max(dot(normalize(vec3(1.0, 1.0, 1.0)), normal), 0.0);
  color_out =
      vec4(material.albedo_color * light, 1.0) *
      texture(texture_samplers[nonuniformEXT(material.albedo_texture_id)],
              tex_coord);
  instance_out = uvec4(instance_id);
}
