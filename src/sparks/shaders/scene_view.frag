#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable
#include "material.glsl"
#include "uniform_objects.glsl"

layout(location = 0) in flat int instance_id;
layout(location = 1) in vec3 position;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec2 tex_coord;
layout(location = 0) out vec4 color_out;
layout(location = 1) out uvec4 instance_out;

layout(binding = 0) readonly uniform global_uniform_object {
  GlobalUniformObject global_object;
};
layout(binding = 2) readonly buffer material_uniform_object {
  Material materials[];
};
layout(binding = 3) uniform sampler2D[] texture_samplers;

#include "sample_texture.glsl"

void main() {
  Material material = materials[instance_id];
  vec3 light = global_object.envmap_minor_color;
  light = max(light, vec3(0.1));
  float sin_offset = sin(global_object.envmap_offset);
  float cos_offset = cos(global_object.envmap_offset);
  light += global_object.envmap_major_color *
           max(dot(global_object.envmap_light_direction, normal), 0.0) * 2.0;
  if (material.material_type == MATERIAL_TYPE_EMISSION) {
    color_out = vec4(material.emission_strength * material.emission, 1.0);
  } else {
    color_out = vec4(material.albedo_color * light, 1.0) *
                SampleTexture(material.albedo_texture_id, tex_coord);
  }
  instance_out = uvec4(instance_id);
}
