#version 450
#extension GL_GOOGLE_include_directive : require
#include "uniform_objects.glsl"

layout(location = 0) in vec2 tex_coord;

layout(binding = 0) uniform global_uniform_object {
  GlobalUniformObject global_object;
};
layout(binding = 1, r32ui) uniform uimage2D stencil_image;
layout(binding = 2) uniform sampler2D color_image;

layout(location = 0) out vec4 out_color;

void main() {
  ivec2 sz = imageSize(stencil_image);
  ivec2 xy = ivec2(tex_coord * sz);
  xy = max(min(xy, sz - ivec2(1, 1)), ivec2(0, 0));
  ivec2 d[4] = {ivec2(0, 1), ivec2(1, 0), ivec2(0, -1), ivec2(-1, 0)};
  out_color = texture(color_image, tex_coord);
  uint self_stencil = imageLoad(stencil_image, xy).r;
  if (global_object.hover_id != -1) {
    if (self_stencil == global_object.hover_id) {
      for (int i = 0; i < 4; i++) {
        ivec2 neighbor = xy + d[i];
        neighbor = max(min(neighbor, sz - ivec2(1, 1)), ivec2(0, 0));
        uint neighbor_stencil = imageLoad(stencil_image, neighbor).r;
        if (neighbor_stencil != self_stencil) {
          out_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
      }
    }
  }
  if (global_object.selected_id != -1) {
    if (self_stencil == global_object.selected_id) {
      for (int i = 0; i < 4; i++) {
        ivec2 neighbor = xy + d[i];
        neighbor = max(min(neighbor, sz - ivec2(1, 1)), ivec2(0, 0));
        uint neighbor_stencil = imageLoad(stencil_image, neighbor).r;
        if (neighbor_stencil != self_stencil) {
          out_color = vec4(0.0, 1.0, 0.0, 1.0);
        }
      }
    }
  }
  out_color = vec4(pow(vec3(out_color), vec3(1.0 / global_object.gamma)), 1.0);
}
