#version 450
layout(location = 0) in vec2 tex_coord;
layout(location = 0) out vec2 frag_tex_coord;

void main() {
  frag_tex_coord = tex_coord;
  gl_Position = vec4(tex_coord * 2.0 - 1.0, 0.0, 1.0);
}
