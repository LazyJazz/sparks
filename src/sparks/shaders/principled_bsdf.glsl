#ifndef PRINCIPLED_BSDF_GLSL
#define PRINCIPLED_BSDF_GLSL

#include "hit_record.glsl"
#include "principled_diffuse.glsl"
#include "random.glsl"

#ifndef Spectrum
#define Spectrum vec3
#endif
#ifndef float3
#define float3 vec3
#endif
#ifndef float2
#define float2 vec2
#endif

PrincipledDiffuseBsdf diffuse_closure;

void CalculateClosureWeight(vec3 base_color,

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
  const vec3 Ng = hit_record.geometry_normal;
  const vec3 N = hit_record.normal;
  const vec3 V = hit_record.omega_v;
  const vec3 I = hit_record.omega_v;
  ior = hit_record.front_face ? 1.0f / ior : ior;

  // calculate fresnel for refraction
  float cosNO = dot(N, I);
  float fresnel = fresnel_dielectric_cos(cosNO, ior);

  // calculate weights of the diffuse and specular part
  float diffuse_weight =
      (1.0f - saturatef(metallic)) * (1.0f - saturatef(transmission));

  float final_transmission =
      saturatef(transmission) * (1.0f - saturatef(metallic));
  float specular_weight = (1.0f - final_transmission);
  float3 clearcoat_normal = N;
  Spectrum weight = vec3(1.0);

  if (diffuse_weight > CLOSURE_WEIGHT_CUTOFF) {
    Spectrum diff_weight = weight * base_color * diffuse_weight;

    PREPARE_BSDF(diffuse_closure, diff_weight);

    if (diffuse_closure.sample_weight > 0.0) {
      diffuse_closure.N = N;
      diffuse_closure.roughness = roughness;
    }
  }
}

vec3 EvalPrincpledBSDF(in vec3 omega_in,
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
  const vec3 Ng = hit_record.geometry_normal;
  const vec3 N = hit_record.normal;
  const vec3 V = hit_record.omega_v;
  const vec3 I = hit_record.omega_v;
  CalculateClosureWeight(base_color, subsurface_color, subsurface,
                         subsurface_radius, metallic, specular, specular_tint,
                         roughness, anisotropic, anisotropic_rotation, sheen,
                         sheen_tint, clearcoat, clearcoat_roughness, ior,
                         transmission, transmission_roughness);
  return bsdf_principled_diffuse_eval(diffuse_closure, I, omega_in, pdf) *
         diffuse_closure.weight;

  // Lambertian
  //  float cos_pi = max(dot(N, omega_in), 0.0f) * INV_PI;
  //  pdf = cos_pi;
  //  return vec3(cos_pi) * hit_record.base_color;
}

void SamplePrincipledBSDF(out vec3 eval,
                          out vec3 omega_in,
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
  const vec3 Ng = hit_record.geometry_normal;
  const vec3 N = hit_record.normal;
  const vec3 V = hit_record.omega_v;
  const vec3 I = hit_record.omega_v;
  CalculateClosureWeight(base_color, subsurface_color, subsurface,
                         subsurface_radius, metallic, specular, specular_tint,
                         roughness, anisotropic, anisotropic_rotation, sheen,
                         sheen_tint, clearcoat, clearcoat_roughness, ior,
                         transmission, transmission_roughness);
  bsdf_principled_diffuse_sample(diffuse_closure, Ng, I, RandomFloat(),
                                 RandomFloat(), eval, omega_in, pdf);
  eval *= diffuse_closure.weight;
  //    const vec3 N = hit_record.normal;
  //    const vec3 Ng = hit_record.geometry_normal;
  //  SampleCosHemisphere(N, L, pdf);
  //  if (dot(Ng, L) > 0.0) {
  //    eval = vec3(pdf) * hit_record.base_color;
  //  } else {
  //    pdf = 0.0f;
  //    eval = vec3(0.0);
  //  }
}

#endif
