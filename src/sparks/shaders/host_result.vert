#version 450

layout(location = 0) in vec2 tex_coord;

void main() {
  gl_Position = vec4(tex_coord * 2.0 - 1.0, 0.0, 1.0);
}
