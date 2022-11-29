#version 450
#extension GL_GOOGLE_include_directive : require
#include "defs.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 tex_coord;

layout(binding = 0) readonly uniform global_uniform_object {
  GlobalUniformObject global_object;
};

layout(binding = 1) readonly buffer entity_uniform_object {
  EntityUniformObject entity_objects[];
};

void main() {
  EntityUniformObject entity_object = entity_objects[gl_InstanceIndex];
  gl_Position = global_object.projection * global_object.camera *
                entity_object.model * vec4(position, 1.0);
}
