#version 450

layout(location = 0) in vec2 vert_pos;
layout(location = 1) in vec4 vert_color;

layout(location = 0) out vec4 frag_color;

void main() {
  gl_Position = vec4(vert_pos, 0.0f, 1.0f);
  frag_color = vert_color;
}
