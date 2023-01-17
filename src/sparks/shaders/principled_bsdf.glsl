#ifndef PRINCIPLED_BSDF_GLSL
#define PRINCIPLED_BSDF_GLSL

#define Ng hit_record.geometry_normal
#define N hit_record.normal
#define V hit_record.omega_v
#define I hit_record.omega_v
#define omega_in L

#include "hit_record.glsl"
#include "random.glsl"

vec3 EvalPrincpledBSDF(in vec3 L,
                       out float pdf,
                       vec3 base_color,

                       vec3 subsurface_color,
                       float subsurface,

                       vec3 subsurface_radius,
                       float metallic,

                       float specular,
                       float specular_tint,
                       float roughness,
                       float anisotropic,

                       float anisotropic_rotation,
                       float sheen,
                       float sheen_tint,
                       float clearcoat,

                       float clearcoat_roughness,
                       float ior,
                       float transmission,
                       float transmission_roughness) {
  float cos_pi = max(dot(N, omega_in), 0.0f) * INV_PI;
  pdf = cos_pi;
  return vec3(cos_pi) * hit_record.base_color;
}

void SamplePrincipledBSDF(out vec3 eval,
                          out vec3 L,
                          out float pdf,
                          vec3 base_color,

                          vec3 subsurface_color,
                          float subsurface,

                          vec3 subsurface_radius,
                          float metallic,

                          float specular,
                          float specular_tint,
                          float roughness,
                          float anisotropic,

                          float anisotropic_rotation,
                          float sheen,
                          float sheen_tint,
                          float clearcoat,

                          float clearcoat_roughness,
                          float ior,
                          float transmission,
                          float transmission_roughness) {
  SampleCosHemisphere(N, L, pdf);
  if (dot(Ng, L) > 0.0) {
    eval = vec3(pdf) * hit_record.base_color;
  } else {
    pdf = 0.0f;
    eval = vec3(0.0);
  }
}

#undef Ng
#undef N
#undef V
#undef I
#undef omega_in

#endif
