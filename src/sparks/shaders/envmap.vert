#version 450
#extension GL_GOOGLE_include_directive : require
#include "material.glsl"
#include "uniform_objects.glsl"

layout(location = 0) in vec2 tex_coord;
layout(location = 0) out vec2 frag_tex_coord;

void main() {
  frag_tex_coord = tex_coord * 2.0 - 1.0;
  gl_Position = vec4(frag_tex_coord, 0.0, 1.0);
}
