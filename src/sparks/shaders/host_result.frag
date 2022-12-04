#version 450

layout(binding = 0, r32f) uniform image2D accumulation_number;
layout(binding = 1, rgba32f) uniform image2D accumulation_color;

layout(location = 0) out vec4 out_color;

void main() {
  ivec2 pos = ivec2(gl_FragCoord.xy);
  ivec2 size = imageSize(accumulation_color);
  pos = max(ivec2(0), min(pos, size - ivec2(1, 1)));
  out_color = vec4(imageLoad(accumulation_color, pos).xyz /
                       max(imageLoad(accumulation_number, pos).r, 1.0),
                   1.0);
}
