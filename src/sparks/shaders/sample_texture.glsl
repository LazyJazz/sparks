#ifndef SAMPLE_TEXTURE_GLSL
#define SAMPLE_TEXTURE_GLSL

vec4 SampleTexture(int texture_id, vec2 tex_coord) {
  return texture(texture_samplers[texture_id], tex_coord);
}

#endif
