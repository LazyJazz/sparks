#version 450
#extension GL_GOOGLE_include_directive : require
#include "defs.glsl"
#include "material.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 tex_coord;

layout(location = 0) out int instance_id;
layout(location = 1) out vec3 frag_position;
layout(location = 2) out vec3 frag_normal;
layout(location = 3) out vec3 frag_tangent;
layout(location = 4) out vec2 frag_tex_coord;

layout(binding = 0) readonly uniform global_uniform_object {
  GlobalUniformObject global_object;
};
layout(binding = 1) readonly buffer entity_uniform_object {
  EntityUniformObject entity_objects[];
};
layout(binding = 2) readonly buffer material_uniform_object {
  Material materials[];
};

void main() {
  EntityUniformObject entity_object = entity_objects[gl_InstanceIndex];
  frag_position = vec3(entity_object.model * vec4(position, 1.0));
  frag_normal = normalize(
      vec3(inverse(transpose(entity_object.model)) * vec4(normal, 0.0)));
  frag_tangent = normalize(vec3(entity_object.model * vec4(tangent, 0.0)));
  frag_tex_coord = tex_coord;
  instance_id = gl_InstanceIndex;
  gl_Position = global_object.projection * global_object.camera *
                vec4(frag_position, 1.0);
}
