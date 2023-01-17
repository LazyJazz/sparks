#ifndef PRINCIPLED_UTIL_GLSL
#define PRINCIPLED_UTIL_GLSL

#ifndef Spectrum
#define Spectrum vec3
#endif
#ifndef float3
#define float3 vec3
#endif
#ifndef float2
#define float2 vec2
#endif

#define CLOSURE_WEIGHT_CUTOFF 1e-5f
#define saturatef(x) clamp(x, 0.0, 1.0)
#define PREPARE_BSDF(sc, weight_in)                 \
  Spectrum weight = weight_in;                      \
  weight = max(weight, vec3(0.0));                  \
  const float sample_weight = abs(average(weight)); \
  if (sample_weight >= CLOSURE_WEIGHT_CUTOFF) {     \
    sc.weight = weight;                             \
    sc.sample_weight = sample_weight;               \
  } else {                                          \
    sc.sample_weight = 0.0;                         \
  }

float average(vec3 v) {
  return (v.x + v.y + v.z) / 3.0f;
}

float fresnel_dielectric_cos(float cosi, float eta) {
  float c = abs(cosi);
  float g = eta * eta - 1 + c * c;
  if (g > 0) {
    g = sqrt(g);
    float A = (g - c) / (g + c);
    float B = (c * (g + c) - 1) / (c * (g - c) + 1);
    return 0.5f * A * A * (1 + B * B);
  }
  return 1.0f;
}

float schlick_fresnel(float u) {
  float m = clamp(1.0f - u, 0.0f, 1.0f);
  float m2 = m * m;
  return m2 * m2 * m;  // pow(m, 5)
}

#endif
