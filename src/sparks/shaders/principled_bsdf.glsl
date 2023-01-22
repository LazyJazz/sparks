#ifndef PRINCIPLED_BSDF_GLSL
#define PRINCIPLED_BSDF_GLSL

#include "hit_record.glsl"
#include "principled_diffuse.glsl"
#include "principled_microfacet.glsl"
#include "principled_microfacet_clearcoat.glsl"
#include "principled_microfacet_fresnel.glsl"
#include "principled_microfacet_refraction.glsl"
#include "principled_sheen.glsl"
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

#define CLOSURE_COUNT 6
PrincipledDiffuseBsdf diffuse_closure;
FresnelBsdf microfacet_closure;
FresnelBsdf microfacet_bsdf_reflect_closure;
RefractionBsdf microfacet_bsdf_refract_closure;
ClearcoatBsdf microfacet_clearcoat_closure;
PrincipledSheenBsdf sheen_closure;

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
  diffuse_closure.weight = vec3(0);
  diffuse_closure.sample_weight = 0.0;
  diffuse_closure.N = vec3(0);
  diffuse_closure.roughness = 0.0;
  microfacet_closure.weight = vec3(0);
  microfacet_closure.sample_weight = 0.0;
  microfacet_closure.N = vec3(0);
  microfacet_closure.alpha_x = 0.0;
  microfacet_closure.alpha_y = 0.0;
  microfacet_closure.ior = 1.0;
  microfacet_closure.T = vec3(0);
  microfacet_closure.color = vec3(0);
  microfacet_closure.cspec0 = vec3(0);
  microfacet_closure.fresnel_color = vec3(0);
  microfacet_closure.clearcoat = 0.0;
  microfacet_closure.type = 0;

  microfacet_bsdf_reflect_closure.weight = vec3(0);
  microfacet_bsdf_reflect_closure.sample_weight = 0.0;
  microfacet_bsdf_reflect_closure.N = vec3(0);
  microfacet_bsdf_reflect_closure.alpha_x = 0.0;
  microfacet_bsdf_reflect_closure.alpha_y = 0.0;
  microfacet_bsdf_reflect_closure.ior = 1.0;
  microfacet_bsdf_reflect_closure.T = vec3(0);
  microfacet_bsdf_reflect_closure.color = vec3(0);
  microfacet_bsdf_reflect_closure.cspec0 = vec3(0);
  microfacet_bsdf_reflect_closure.fresnel_color = vec3(0);
  microfacet_bsdf_reflect_closure.clearcoat = 0.0;
  microfacet_bsdf_reflect_closure.type = 0;

  microfacet_bsdf_refract_closure.weight = vec3(0);
  microfacet_bsdf_refract_closure.sample_weight = 0.0;
  microfacet_bsdf_refract_closure.N = vec3(0);
  microfacet_bsdf_refract_closure.alpha_x = 0.0;
  microfacet_bsdf_refract_closure.alpha_y = 0.0;
  microfacet_bsdf_refract_closure.ior = 1.0;
  microfacet_bsdf_refract_closure.T = vec3(0);
  microfacet_bsdf_refract_closure.color = vec3(0);
  microfacet_bsdf_refract_closure.cspec0 = vec3(0);
  microfacet_bsdf_refract_closure.fresnel_color = vec3(0);
  microfacet_bsdf_refract_closure.clearcoat = 0.0;
  microfacet_bsdf_refract_closure.type = 0;

  microfacet_clearcoat_closure.weight = vec3(0);
  microfacet_clearcoat_closure.sample_weight = 0.0;
  microfacet_clearcoat_closure.N = vec3(0);
  microfacet_clearcoat_closure.alpha_x = 0.0;
  microfacet_clearcoat_closure.alpha_y = 0.0;
  microfacet_clearcoat_closure.ior = 1.0;
  microfacet_clearcoat_closure.T = vec3(0);
  microfacet_clearcoat_closure.color = vec3(0);
  microfacet_clearcoat_closure.cspec0 = vec3(0);
  microfacet_clearcoat_closure.fresnel_color = vec3(0);
  microfacet_clearcoat_closure.clearcoat = 0.0;
  microfacet_clearcoat_closure.type = 0;

  sheen_closure.weight = vec3(0);
  sheen_closure.sample_weight = 0.0;
  sheen_closure.N = vec3(0);
  sheen_closure.avg_value = 0.0;

  const vec3 Ng = hit_record.geometry_normal;
  const vec3 N = hit_record.normal;
  const vec3 V = hit_record.omega_v;
  const vec3 I = hit_record.omega_v;
  vec3 T = hit_record.tangent;
  if (anisotropic_rotation != 0.0f)
    T = rotate_around_axis(T, N, anisotropic_rotation * 2.0 * PI);
  ior = hit_record.front_face ? ior : 1.0f / ior;

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

  if (diffuse_weight > CLOSURE_WEIGHT_CUTOFF && sheen > CLOSURE_WEIGHT_CUTOFF) {
    float m_cdlum = dot(vec3(0.2126729f, 0.7151522f, 0.0721750f), base_color);
    float3 m_ctint = m_cdlum > 0.0f
                         ? base_color / m_cdlum
                         : vec3(1);  // normalize lum. to isolate hue+sat

    /* color of the sheen component */
    float3 sheen_color = vec3(1.0f - sheen_tint) + m_ctint * sheen_tint;

    Spectrum sheen_weight = weight * sheen * sheen_color * diffuse_weight;

    PREPARE_BSDF(sheen_closure, sheen_weight);

    {
      sheen_closure.N = N;
      bsdf_principled_sheen_setup(sheen_closure);
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

  if (final_transmission > CLOSURE_WEIGHT_CUTOFF) {
    Spectrum glass_weight = weight * final_transmission;
    float3 cspec0 = base_color * specular_tint + vec3(1.0f - specular_tint);
    float refl_roughness = roughness;

    /* reflection */
    {
      PREPARE_BSDF(microfacet_bsdf_reflect_closure, glass_weight * fresnel);

      {
        microfacet_bsdf_reflect_closure.N = N;
        microfacet_bsdf_reflect_closure.T = vec3(0);

        microfacet_bsdf_reflect_closure.alpha_x =
            refl_roughness * refl_roughness;
        microfacet_bsdf_reflect_closure.alpha_y =
            refl_roughness * refl_roughness;
        microfacet_bsdf_reflect_closure.ior = ior;

        microfacet_bsdf_reflect_closure.color = base_color;
        microfacet_bsdf_reflect_closure.cspec0 = cspec0;
        microfacet_bsdf_reflect_closure.clearcoat = 0.0f;

        /* setup bsdf */
        bsdf_microfacet_ggx_fresnel_setup(microfacet_bsdf_reflect_closure);
      }
    }

    /* refraction */
    {
      /* This is to prevent MNEE from receiving a null BSDF. */
      float refraction_fresnel = max(0.0001f, 1.0f - fresnel);
      PREPARE_BSDF(microfacet_bsdf_refract_closure,
                   base_color * glass_weight * refraction_fresnel);
      {
        microfacet_bsdf_refract_closure.N = N;
        microfacet_bsdf_refract_closure.T = vec3(0);

        transmission_roughness =
            1.0f - (1.0f - refl_roughness) * (1.0f - transmission_roughness);

        microfacet_bsdf_refract_closure.alpha_x =
            transmission_roughness * transmission_roughness;
        microfacet_bsdf_refract_closure.alpha_y =
            transmission_roughness * transmission_roughness;
        microfacet_bsdf_refract_closure.ior = ior;

        /* setup bsdf */
        bsdf_microfacet_ggx_refraction_setup(microfacet_bsdf_refract_closure);
      }
    }
  }

  if (clearcoat > CLOSURE_WEIGHT_CUTOFF) {
    PREPARE_BSDF(microfacet_clearcoat_closure, weight);

    {
      microfacet_clearcoat_closure.N = clearcoat_normal;
      microfacet_clearcoat_closure.T = vec3(0);
      microfacet_clearcoat_closure.ior = 1.5f;

      microfacet_clearcoat_closure.alpha_x =
          clearcoat_roughness * clearcoat_roughness;
      microfacet_clearcoat_closure.alpha_y =
          clearcoat_roughness * clearcoat_roughness;

      microfacet_clearcoat_closure.color = vec3(0);
      microfacet_clearcoat_closure.cspec0 = vec3(0.04f);
      microfacet_clearcoat_closure.clearcoat = clearcoat;

      /* setup bsdf */
      bsdf_microfacet_ggx_clearcoat_setup(microfacet_clearcoat_closure);
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
    eval += bsdf_principled_diffuse_eval(diffuse_closure, hit_record.omega_v,
                                         omega_in, local_pdf) *
            diffuse_closure.weight;
    pdf += local_pdf * diffuse_closure.sample_weight;
    accum_weight += diffuse_closure.sample_weight;
  }
  if (exclude != 1 &&
      microfacet_closure.sample_weight >= CLOSURE_WEIGHT_CUTOFF) {
    float local_pdf;
    eval += bsdf_microfacet_ggx_eval_fresnel(
                microfacet_closure, hit_record.omega_v, omega_in, local_pdf) *
            microfacet_closure.weight;
    pdf += local_pdf * microfacet_closure.sample_weight;
    accum_weight += microfacet_closure.sample_weight;
  }
  if (exclude != 2 &&
      microfacet_bsdf_reflect_closure.sample_weight >= CLOSURE_WEIGHT_CUTOFF) {
    float local_pdf;
    eval += bsdf_microfacet_ggx_eval_fresnel(microfacet_bsdf_reflect_closure,
                                             hit_record.omega_v, omega_in,
                                             local_pdf) *
            microfacet_bsdf_reflect_closure.weight;
    pdf += local_pdf * microfacet_bsdf_reflect_closure.sample_weight;
    accum_weight += microfacet_bsdf_reflect_closure.sample_weight;
  }

  if (exclude != 3 &&
      microfacet_bsdf_refract_closure.sample_weight >= CLOSURE_WEIGHT_CUTOFF) {
    float local_pdf;
    eval += bsdf_microfacet_ggx_eval_refraction(microfacet_bsdf_refract_closure,
                                                hit_record.omega_v, omega_in,
                                                local_pdf) *
            microfacet_bsdf_refract_closure.weight;
    pdf += local_pdf * microfacet_bsdf_refract_closure.sample_weight;
    accum_weight += microfacet_bsdf_refract_closure.sample_weight;
  }
  if (exclude != 4 &&
      microfacet_clearcoat_closure.sample_weight >= CLOSURE_WEIGHT_CUTOFF) {
    float local_pdf;
    eval += bsdf_microfacet_ggx_eval_clearcoat(microfacet_clearcoat_closure,
                                               hit_record.omega_v, omega_in,
                                               local_pdf) *
            microfacet_clearcoat_closure.weight;
    pdf += local_pdf * microfacet_clearcoat_closure.sample_weight;
    accum_weight += microfacet_clearcoat_closure.sample_weight;
  }
  if (exclude != 5 && sheen_closure.sample_weight >= CLOSURE_WEIGHT_CUTOFF) {
    float local_pdf;
    vec3 local_eval =
        bsdf_principled_sheen_eval(sheen_closure, hit_record.omega_v, omega_in,
                                   local_pdf) *
        sheen_closure.weight;
    pdf += local_pdf * sheen_closure.sample_weight;
    eval += local_eval;
    accum_weight += sheen_closure.sample_weight;
  }
  if (accum_weight < CLOSURE_WEIGHT_CUTOFF) {
    pdf = 0.0;
    return eval;
  }
  pdf /= accum_weight;
  return eval;
}

vec3 EvalPrincipledBSDF(in vec3 omega_in, out float pdf) {
  CalculateClosureWeight(
      hit_record.base_color, hit_record.subsurface_color, hit_record.subsurface,
      hit_record.subsurface_radius, hit_record.metallic, hit_record.specular,
      hit_record.specular_tint, hit_record.roughness, hit_record.anisotropic,
      hit_record.anisotropic_rotation, hit_record.sheen, hit_record.sheen_tint,
      hit_record.clearcoat, hit_record.clearcoat_roughness, hit_record.ior,
      hit_record.transmission, hit_record.transmission_roughness);
  pdf = 0.0;
  return EvalPrincipledBSDFKernel(omega_in, pdf, vec3(0.0), 0.0, -1);
}

void SamplePrincipledBSDF(out vec3 eval, out vec3 omega_in, out float pdf) {
  eval = vec3(0);
  omega_in = vec3(0);
  pdf = 0.0;
  const vec3 Ng = hit_record.geometry_normal;
  const vec3 N = hit_record.normal;
  const vec3 I = hit_record.omega_v;

  CalculateClosureWeight(
      hit_record.base_color, hit_record.subsurface_color, hit_record.subsurface,
      hit_record.subsurface_radius, hit_record.metallic, hit_record.specular,
      hit_record.specular_tint, hit_record.roughness, hit_record.anisotropic,
      hit_record.anisotropic_rotation, hit_record.sheen, hit_record.sheen_tint,
      hit_record.clearcoat, hit_record.clearcoat_roughness, hit_record.ior,
      hit_record.transmission, hit_record.transmission_roughness);
  float weight_cdf[CLOSURE_COUNT];
  float total_cdf;
  weight_cdf[0] = diffuse_closure.sample_weight;
  weight_cdf[1] = microfacet_closure.sample_weight + weight_cdf[0];
  weight_cdf[2] = microfacet_bsdf_reflect_closure.sample_weight + weight_cdf[1];
  weight_cdf[3] = microfacet_bsdf_refract_closure.sample_weight + weight_cdf[2];
  weight_cdf[4] = microfacet_clearcoat_closure.sample_weight + weight_cdf[3];
  weight_cdf[5] = sheen_closure.sample_weight + weight_cdf[4];
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
    bsdf_microfacet_ggx_sample_fresnel(microfacet_closure, N, I, r1, r2, eval,
                                       omega_in, pdf, sampled_roughness, eta);
    eval *= microfacet_closure.weight;
    exclude = 1;
    accum_weight = microfacet_closure.sample_weight;
  } else if (r1 < weight_cdf[2]) {
    r1 -= weight_cdf[1];
    r1 /= weight_cdf[2] - weight_cdf[1];
    vec2 sampled_roughness;
    float eta;
    bsdf_microfacet_ggx_sample_fresnel(microfacet_bsdf_reflect_closure, N, I,
                                       r1, r2, eval, omega_in, pdf,
                                       sampled_roughness, eta);
    eval *= microfacet_bsdf_reflect_closure.weight;
    exclude = 2;
    accum_weight = microfacet_bsdf_reflect_closure.sample_weight;
  } else if (r1 < weight_cdf[3]) {
    r1 -= weight_cdf[2];
    r1 /= weight_cdf[3] - weight_cdf[2];
    vec2 sampled_roughness;
    float eta;
    bsdf_microfacet_ggx_sample_refraction(microfacet_bsdf_refract_closure, N, I,
                                          r1, r2, eval, omega_in, pdf,
                                          sampled_roughness, eta);
    eval *= microfacet_bsdf_refract_closure.weight;
    exclude = 3;
    accum_weight = microfacet_bsdf_refract_closure.sample_weight;
  } else if (r1 < weight_cdf[4]) {
    r1 -= weight_cdf[3];
    r1 /= weight_cdf[4] - weight_cdf[3];
    vec2 sampled_roughness;
    float eta;
    bsdf_microfacet_ggx_sample_clearcoat(microfacet_clearcoat_closure, N, I, r1,
                                         r2, eval, omega_in, pdf,
                                         sampled_roughness, eta);
    eval *= microfacet_clearcoat_closure.weight;
    exclude = 4;
    accum_weight = microfacet_clearcoat_closure.sample_weight;
  } else if (r1 < weight_cdf[5]) {
    r1 -= weight_cdf[4];
    r1 /= weight_cdf[5] - weight_cdf[4];
    vec2 sampled_roughness;
    float eta;
    bsdf_principled_sheen_sample(sheen_closure, N, I, r1, r2, eval, omega_in,
                                 pdf);
    eval *= sheen_closure.weight;
    exclude = 5;
    accum_weight = sheen_closure.sample_weight;
  }
  pdf *= accum_weight;
  eval = EvalPrincipledBSDFKernel(omega_in, pdf, eval, accum_weight, exclude);
}

#endif
