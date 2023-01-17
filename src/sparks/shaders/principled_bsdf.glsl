#ifndef PRINCIPLED_BSDF_GLSL
#define PRINCIPLED_BSDF_GLSL

#include "hit_record.glsl"
#include "principled_diffuse.glsl"
#include "principled_microfacet.glsl"
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

#define CLOSURE_COUNT 2
PrincipledDiffuseBsdf diffuse_closure;
MicrofacetBsdf microfacet_closure;

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
  diffuse_closure.sample_weight = 0.0;
  microfacet_closure.sample_weight = 0.0;
  const vec3 Ng = hit_record.geometry_normal;
  const vec3 N = hit_record.normal;
  const vec3 V = hit_record.omega_v;
  const vec3 I = hit_record.omega_v;
  vec3 T = hit_record.tangent;
  if (anisotropic_rotation != 0.0f)
    T = rotate_around_axis(T, N, anisotropic_rotation * 2.0 * PI);
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

  if (specular_weight > CLOSURE_WEIGHT_CUTOFF &&
      (specular > CLOSURE_WEIGHT_CUTOFF || metallic > CLOSURE_WEIGHT_CUTOFF)) {
    Spectrum spec_weight = weight * specular_weight;

    PREPARE_BSDF(microfacet_closure, spec_weight);

    {
      microfacet_closure.N = N;
      microfacet_closure.ior =
          (2.0f / (1.0f - safe_sqrtf(0.08f * specular))) - 1.0f;
      microfacet_closure.T = T;

      float aspect = safe_sqrtf(1.0f - anisotropic * 0.9f);
      float r2 = roughness * roughness;

      microfacet_closure.alpha_x = r2 / aspect;
      microfacet_closure.alpha_y = r2 * aspect;

      float m_cdlum = 0.3f * base_color.x + 0.6f * base_color.y +
                      0.1f * base_color.z;  // luminance approx.
      float3 m_ctint = m_cdlum > 0.0f
                           ? base_color / m_cdlum
                           : vec3(1.0);  // normalize lum. to isolate hue+sat
      float3 tmp_col = vec3(1.0f - specular_tint) + m_ctint * specular_tint;

      microfacet_closure.cspec0 =
          ((specular * 0.08f * tmp_col) * (1.0f - metallic) +
           base_color * metallic);
      microfacet_closure.color = (base_color);
      microfacet_closure.clearcoat = 0.0f;

      bsdf_microfacet_ggx_fresnel_setup(microfacet_closure);
    }
  }
}

vec3 EvalPrincipledBSDFKernel(in vec3 omega_in,
                              inout float pdf,
                              in vec3 eval,
                              in float accum_weight,
                              int exclude) {
  if (exclude != 0 && diffuse_closure.sample_weight >= CLOSURE_WEIGHT_CUTOFF) {
    float local_pdf;
    vec3 local_eval =
        bsdf_principled_diffuse_eval(diffuse_closure, hit_record.omega_v,
                                     omega_in, local_pdf) *
        diffuse_closure.weight * diffuse_closure.sample_weight;
    pdf += local_pdf * diffuse_closure.sample_weight;
    eval += local_eval * diffuse_closure.sample_weight;
    accum_weight += diffuse_closure.sample_weight;
  }
  if (exclude != 1 &&
      microfacet_closure.sample_weight >= CLOSURE_WEIGHT_CUTOFF) {
    float local_pdf;
    vec3 local_eval =
        bsdf_microfacet_ggx_eval(microfacet_closure, hit_record.omega_v,
                                 omega_in, local_pdf) *
        microfacet_closure.weight * microfacet_closure.sample_weight;
    pdf += local_pdf * microfacet_closure.sample_weight;
    eval += local_eval * microfacet_closure.sample_weight;
    accum_weight += microfacet_closure.sample_weight;
  }
  if (accum_weight < CLOSURE_WEIGHT_CUTOFF) {
    return eval;
  }
  pdf /= accum_weight;
  return eval /= accum_weight;
}

vec3 EvalPrincipledBSDF(in vec3 omega_in,
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
  CalculateClosureWeight(base_color, subsurface_color, subsurface,
                         subsurface_radius, metallic, specular, specular_tint,
                         roughness, anisotropic, anisotropic_rotation, sheen,
                         sheen_tint, clearcoat, clearcoat_roughness, ior,
                         transmission, transmission_roughness);
  pdf = 0.0;
  return EvalPrincipledBSDFKernel(omega_in, pdf, vec3(0.0), 0.0, -1);
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
  float weight_cdf[CLOSURE_COUNT];
  float total_cdf;
  weight_cdf[0] = diffuse_closure.sample_weight;
  weight_cdf[1] = microfacet_closure.sample_weight + weight_cdf[0];
  total_cdf = weight_cdf[CLOSURE_COUNT - 1];
  for (int i = 0; i < CLOSURE_COUNT; i++) {
    weight_cdf[i] /= total_cdf;
  }
  float r1 = RandomFloat();
  float r2 = RandomFloat();
  int exclude = -1;
  float accum_weight = 0.0;
  if (r1 < weight_cdf[0]) {
    r1 /= weight_cdf[0];
    bsdf_principled_diffuse_sample(diffuse_closure, Ng, I, r1, r2, eval,
                                   omega_in, pdf);
    eval *= diffuse_closure.weight;
    exclude = 0;
    accum_weight = diffuse_closure.sample_weight;
  } else if (r1 < weight_cdf[1]) {
    r1 -= weight_cdf[0];
    r1 /= weight_cdf[1] - weight_cdf[0];
    vec2 sampled_roughness;
    float eta;
    bsdf_microfacet_ggx_sample(microfacet_closure, hit_record.geometry_normal,
                               hit_record.omega_v, r1, r2, eval, omega_in, pdf,
                               sampled_roughness, eta);
    eval *= microfacet_closure.weight;
    exclude = 1;
    accum_weight = microfacet_closure.sample_weight;
  }
  //    const vec3 N = hit_record.normal;
  //    const vec3 Ng = hit_record.geometry_normal;
  //  SampleCosHemisphere(N, L, pdf);
  //  if (dot(Ng, L) > 0.0) {
  //    eval = vec3(pdf) * hit_record.base_color;
  //  } else {
  //    pdf = 0.0f;
  //    eval = vec3(0.0);
  //  }
  eval = EvalPrincipledBSDFKernel(omega_in, pdf, eval, accum_weight, exclude);
}

#endif
