#ifndef PRINCIPLED_MICROFACET_GLSL
#define PRINCIPLED_MICROFACET_GLSL

#include "principled_util.glsl"
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

#define CLOSURE_BSDF_MICROFACET_GGX_FRESNEL_ID 0
#define CLOSURE_BSDF_MICROFACET_GGX_CLEARCOAT_ID 1
#define CLOSURE_BSDF_MICROFACET_GGX_REFRACTION_ID 2
#define CLOSURE_BSDF_MICROFACET_GGX_GLASS_ID 3

struct MicrofacetBsdf {
  Spectrum weight;
  float sample_weight;
  float3 N;
  float alpha_x, alpha_y, ior;
  float3 T;
  Spectrum color, cspec0;
  Spectrum fresnel_color;
  float clearcoat;
  int type;
};

void bsdf_microfacet_fresnel_color(inout MicrofacetBsdf bsdf) {
  float F0 = fresnel_dielectric_cos(1.0f, bsdf.ior);
  bsdf.fresnel_color = interpolate_fresnel_color(hit_record.omega_v, bsdf.N,
                                                 bsdf.ior, F0, bsdf.cspec0);

  if (bsdf.type == CLOSURE_BSDF_MICROFACET_GGX_CLEARCOAT_ID) {
    bsdf.fresnel_color *= 0.25f * bsdf.clearcoat;
  }

  bsdf.sample_weight *= average(bsdf.fresnel_color);
}

Spectrum reflection_color(const MicrofacetBsdf bsdf, float3 L, float3 H) {
  Spectrum F = vec3(1);
  bool use_fresnel = (bsdf.type == CLOSURE_BSDF_MICROFACET_GGX_FRESNEL_ID ||
                      bsdf.type == CLOSURE_BSDF_MICROFACET_GGX_CLEARCOAT_ID);
  if (use_fresnel) {
    float F0 = fresnel_dielectric_cos(1.0f, bsdf.ior);

    F = interpolate_fresnel_color(L, H, bsdf.ior, F0, bsdf.cspec0);
  }

  return F;
}

void microfacet_beckmann_sample_slopes(const float cos_theta_i,
                                       const float sin_theta_i,
                                       float randu,
                                       float randv,
                                       inout float slope_x,
                                       inout float slope_y,
                                       inout float G1i) {
  /* special case (normal incidence) */
  if (cos_theta_i >= 0.99999f) {
    const float r = sqrt(-log(randu));
    const float phi = 2.0 * PI * randv;
    slope_x = r * cos(phi);
    slope_y = r * sin(phi);
    G1i = 1.0f;
    return;
  }

  /* precomputations */
  const float tan_theta_i = sin_theta_i / cos_theta_i;
  const float inv_a = tan_theta_i;
  const float cot_theta_i = 1.0f / tan_theta_i;
  const float erf_a = fast_erff(cot_theta_i);
  const float exp_a2 = exp(-cot_theta_i * cot_theta_i);
  const float SQRT_PI_INV = 0.56418958354f;
  const float Lambda =
      0.5f * (erf_a - 1.0f) + (0.5f * SQRT_PI_INV) * (exp_a2 * inv_a);
  const float G1 = 1.0f / (1.0f + Lambda); /* masking */

  G1i = G1;
  /* Based on paper from Wenzel Jakob
   * An Improved Visible Normal Sampling Routine for the Beckmann Distribution
   *
   * http://www.mitsuba-renderer.org/~wenzel/files/visnormal.pdf
   *
   * Reformulation from OpenShadingLanguage which avoids using inverse
   * trigonometric functions.
   */

  /* Sample slope X.
   *
   * Compute a coarse approximation using the approximation:
   *   exp(-ierf(x)^2) ~= 1 - x * x
   *   solve y = 1 + b + K * (1 - b * b)
   */
  float K = tan_theta_i * SQRT_PI_INV;
  float y_approx = randu * (1.0f + erf_a + K * (1 - erf_a * erf_a));
  float y_exact = randu * (1.0f + erf_a + K * exp_a2);
  float b = K > 0 ? (0.5f - sqrt(K * (K - y_approx + 1.0f) + 0.25f)) / K
                  : y_approx - 1.0f;

  /* Perform newton step to refine toward the true root. */
  float inv_erf = fast_ierff(b);
  float value = 1.0f + b + K * exp(-inv_erf * inv_erf) - y_exact;
  /* Check if we are close enough already,
   * this also avoids NaNs as we get close to the root.
   */
  if (abs(value) > 1e-6f) {
    b -= value / (1.0f - inv_erf * tan_theta_i); /* newton step 1. */
    inv_erf = fast_ierff(b);
    value = 1.0f + b + K * exp(-inv_erf * inv_erf) - y_exact;
    b -= value / (1.0f - inv_erf * tan_theta_i); /* newton step 2. */
    /* Compute the slope from the refined value. */
    slope_x = fast_ierff(b);
  } else {
    /* We are close enough already. */
    slope_x = inv_erf;
  }
  slope_y = fast_ierff(2.0f * randv - 1.0f);
}

void microfacet_ggx_sample_slopes(const float cos_theta_i,
                                  const float sin_theta_i,
                                  float randu,
                                  float randv,
                                  inout float slope_x,
                                  inout float slope_y,
                                  inout float G1i) {
  /* special case (normal incidence) */
  if (cos_theta_i >= 0.99999f) {
    const float r = sqrt(randu / (1.0f - randu));
    const float phi = 2.0 * PI * randv;
    slope_x = r * cos(phi);
    slope_y = r * sin(phi);
    G1i = 1.0f;

    return;
  }

  /* precomputations */
  const float tan_theta_i = sin_theta_i / cos_theta_i;
  const float G1_inv =
      0.5f * (1.0f + safe_sqrtf(1.0f + tan_theta_i * tan_theta_i));

  G1i = 1.0f / G1_inv;

  /* sample slope_x */
  const float A = 2.0f * randu * G1_inv - 1.0f;
  const float AA = A * A;
  const float tmp = 1.0f / (AA - 1.0f);
  const float B = tan_theta_i;
  const float BB = B * B;
  const float D = safe_sqrtf(BB * (tmp * tmp) - (AA - BB) * tmp);
  const float slope_x_1 = B * tmp - D;
  const float slope_x_2 = B * tmp + D;
  slope_x =
      (A < 0.0f || slope_x_2 * tan_theta_i > 1.0f) ? slope_x_1 : slope_x_2;

  /* sample slope_y */
  float S;

  if (randv > 0.5f) {
    S = 1.0f;
    randv = 2.0f * (randv - 0.5f);
  } else {
    S = -1.0f;
    randv = 2.0f * (0.5f - randv);
  }

  const float z =
      (randv * (randv * (randv * 0.27385f - 0.73369f) + 0.46341f)) /
      (randv * (randv * (randv * 0.093073f + 0.309420f) - 1.000000f) +
       0.597999f);
  slope_y = S * z * safe_sqrtf(1.0f + (slope_x) * (slope_x));
}

float3 microfacet_sample_stretched(const float3 omega_i,
                                   const float alpha_x,
                                   const float alpha_y,
                                   const float randu,
                                   const float randv,
                                   bool beckmann,
                                   inout float G1i) {
  /* 1. stretch omega_i */
  float3 omega_i_ = vec3(alpha_x * omega_i.x, alpha_y * omega_i.y, omega_i.z);
  omega_i_ = normalize(omega_i_);

  /* get polar coordinates of omega_i_ */
  float costheta_ = 1.0f;
  float sintheta_ = 0.0f;
  float cosphi_ = 1.0f;
  float sinphi_ = 0.0f;

  if (omega_i_.z < 0.99999f) {
    costheta_ = omega_i_.z;
    sintheta_ = safe_sqrtf(1.0f - costheta_ * costheta_);

    float invlen = 1.0f / sintheta_;
    cosphi_ = omega_i_.x * invlen;
    sinphi_ = omega_i_.y * invlen;
  }

  /* 2. sample P22_{omega_i}(x_slope, y_slope, 1, 1) */
  float slope_x, slope_y;

  if (beckmann) {
    microfacet_beckmann_sample_slopes(costheta_, sintheta_, randu, randv,
                                      slope_x, slope_y, G1i);
  } else {
    microfacet_ggx_sample_slopes(costheta_, sintheta_, randu, randv, slope_x,
                                 slope_y, G1i);
  }

  /* 3. rotate */
  float tmp = cosphi_ * slope_x - sinphi_ * slope_y;
  slope_y = sinphi_ * slope_x + cosphi_ * slope_y;
  slope_x = tmp;

  /* 4. unstretch */
  slope_x = alpha_x * slope_x;
  slope_y = alpha_y * slope_y;

  /* 5. compute normal */
  return normalize(vec3(-slope_x, -slope_y, 1.0f));
}

void bsdf_microfacet_ggx_fresnel_setup(inout MicrofacetBsdf bsdf) {
  bsdf.cspec0 = saturate(bsdf.cspec0);
  bsdf.alpha_x = saturatef(bsdf.alpha_x);
  bsdf.alpha_y = saturatef(bsdf.alpha_y);
  bsdf.type = CLOSURE_BSDF_MICROFACET_GGX_FRESNEL_ID;
  bsdf_microfacet_fresnel_color(bsdf);
}

void bsdf_microfacet_ggx_refraction_setup(inout MicrofacetBsdf bsdf) {
  bsdf.clearcoat = 0.0;
  bsdf.fresnel_color = vec3(0);
  bsdf.color = vec3(0);
  bsdf.cspec0 = vec3(0);
  bsdf.alpha_x = saturatef(bsdf.alpha_x);
  bsdf.alpha_y = bsdf.alpha_x;

  bsdf.type = CLOSURE_BSDF_MICROFACET_GGX_REFRACTION_ID;
}

void bsdf_microfacet_ggx_clearcoat_setup(inout MicrofacetBsdf bsdf) {
  bsdf.cspec0 = saturate(bsdf.cspec0);

  bsdf.alpha_x = saturatef(bsdf.alpha_x);
  bsdf.alpha_y = bsdf.alpha_x;

  bsdf.type = CLOSURE_BSDF_MICROFACET_GGX_CLEARCOAT_ID;

  bsdf_microfacet_fresnel_color(bsdf);
}

Spectrum bsdf_microfacet_ggx_eval_reflect(const MicrofacetBsdf bsdf,
                                          const float3 N,
                                          const float3 I,
                                          const float3 omega_in,
                                          inout float pdf,
                                          const float alpha_x,
                                          const float alpha_y,
                                          const float cosNO,
                                          const float cosNI) {
  if (!(cosNI > 0 && cosNO > 0)) {
    pdf = 0.0f;
    return vec3(0);
  }

  /* get half vector */
  float3 m = normalize(omega_in + I);
  float alpha2 = alpha_x * alpha_y;
  float D, G1o, G1i;

  if (alpha_x == alpha_y) {
    /* isotropic
     * eq. 20: (F*G*D)/(4*in*on)
     * eq. 33: first we calculate D(m) */
    float cosThetaM = dot(N, m);
    float cosThetaM2 = cosThetaM * cosThetaM;
    float cosThetaM4 = cosThetaM2 * cosThetaM2;
    float tanThetaM2 = (1 - cosThetaM2) / cosThetaM2;

    if (bsdf.type == CLOSURE_BSDF_MICROFACET_GGX_CLEARCOAT_ID) {
      /* use GTR1 for clearcoat */
      D = D_GTR1(cosThetaM, bsdf.alpha_x);

      /* the alpha value for clearcoat is a fixed 0.25 => alpha2 = 0.25 * 0.25
       */
      alpha2 = 0.0625f;
    } else {
      /* use GTR2 otherwise */
      D = alpha2 /
          (PI * cosThetaM4 * (alpha2 + tanThetaM2) * (alpha2 + tanThetaM2));
    }

    /* eq. 34: now calculate G1(i,m) and G1(o,m) */
    G1o = 2 /
          (1 + safe_sqrtf(1 + alpha2 * (1 - cosNO * cosNO) / (cosNO * cosNO)));
    G1i = 2 /
          (1 + safe_sqrtf(1 + alpha2 * (1 - cosNI * cosNI) / (cosNI * cosNI)));
  } else {
    /* anisotropic */
    float3 X, Y, Z = N;
    make_orthonormals_tangent(Z, bsdf.T, X, Y);

    /* distribution */
    float3 local_m = vec3(dot(X, m), dot(Y, m), dot(Z, m));
    float slope_x = -local_m.x / (local_m.z * alpha_x);
    float slope_y = -local_m.y / (local_m.z * alpha_y);
    float slope_len = 1 + slope_x * slope_x + slope_y * slope_y;

    float cosThetaM = local_m.z;
    float cosThetaM2 = cosThetaM * cosThetaM;
    float cosThetaM4 = cosThetaM2 * cosThetaM2;

    D = 1 / ((slope_len * slope_len) * PI * alpha2 * cosThetaM4);

    /* G1(i,m) and G1(o,m) */
    float tanThetaO2 = (1 - cosNO * cosNO) / (cosNO * cosNO);
    float cosPhiO = dot(I, X);
    float sinPhiO = dot(I, Y);

    float alphaO2 = (cosPhiO * cosPhiO) * (alpha_x * alpha_x) +
                    (sinPhiO * sinPhiO) * (alpha_y * alpha_y);
    alphaO2 /= cosPhiO * cosPhiO + sinPhiO * sinPhiO;

    G1o = 2 / (1 + safe_sqrtf(1 + alphaO2 * tanThetaO2));

    float tanThetaI2 = (1 - cosNI * cosNI) / (cosNI * cosNI);
    float cosPhiI = dot(omega_in, X);
    float sinPhiI = dot(omega_in, Y);

    float alphaI2 = (cosPhiI * cosPhiI) * (alpha_x * alpha_x) +
                    (sinPhiI * sinPhiI) * (alpha_y * alpha_y);
    alphaI2 /= cosPhiI * cosPhiI + sinPhiI * sinPhiI;

    G1i = 2 / (1 + safe_sqrtf(1 + alphaI2 * tanThetaI2));
  }

  float G = G1o * G1i;

  /* eq. 20 */
  float common_ = D * 0.25f / cosNO;

  Spectrum F = reflection_color(bsdf, omega_in, m);
  if (bsdf.type == CLOSURE_BSDF_MICROFACET_GGX_CLEARCOAT_ID) {
    F *= 0.25f * bsdf.clearcoat;
  }

  Spectrum out_ = F * G * common_;

  /* eq. 2 in distribution of visible normals sampling
   * `pm = Dw = G1o * dot(m, I) * D / dot(N, I);` */

  /* eq. 38 - but see also:
   * eq. 17 in http://www.graphics.cornell.edu/~bjw/wardnotes.pdf
   * `pdf = pm * 0.25 / dot(m, I);` */
  pdf = G1o * common_;

  return out_;
}

Spectrum bsdf_microfacet_ggx_eval_transmit(const MicrofacetBsdf bsdf,
                                           const float3 N,
                                           const float3 I,
                                           const float3 omega_in,
                                           inout float pdf,
                                           const float alpha_x,
                                           const float alpha_y,
                                           const float cosNO,
                                           const float cosNI) {
  if (cosNO <= 0 || cosNI >= 0) {
    pdf = 0.0f;
    return vec3(0); /* vectors on same side -- not possible */
  }
  /* compute half-vector of the refraction (eq. 16) */
  float m_eta = bsdf.ior;
  float3 ht = -(m_eta * omega_in + I);
  float3 Ht = normalize(ht);
  float cosHO = dot(Ht, I);
  float cosHI = dot(Ht, omega_in);

  float D, G1o, G1i;

  /* eq. 33: first we calculate D(m) with m=Ht: */
  float alpha2 = alpha_x * alpha_y;
  float cosThetaM = dot(N, Ht);
  float cosThetaM2 = cosThetaM * cosThetaM;
  float tanThetaM2 = (1 - cosThetaM2) / cosThetaM2;
  float cosThetaM4 = cosThetaM2 * cosThetaM2;
  D = alpha2 /
      (PI * cosThetaM4 * (alpha2 + tanThetaM2) * (alpha2 + tanThetaM2));

  /* eq. 34: now calculate G1(i,m) and G1(o,m) */
  G1o =
      2 / (1 + safe_sqrtf(1 + alpha2 * (1 - cosNO * cosNO) / (cosNO * cosNO)));
  G1i =
      2 / (1 + safe_sqrtf(1 + alpha2 * (1 - cosNI * cosNI) / (cosNI * cosNI)));

  float G = G1o * G1i;

  /* probability */
  float Ht2 = dot(ht, ht);

  /* eq. 2 in distribution of visible normals sampling
   * pm = Dw = G1o * dot(m, I) * D / dot(N, I); */

  /* out = fabsf(cosHI * cosHO) * (m_eta * m_eta) * G * D / (cosNO * Ht2)
   * pdf = pm * (m_eta * m_eta) * fabsf(cosHI) / Ht2 */
  float common_ = D * (m_eta * m_eta) / (cosNO * Ht2);
  float out_ = G * abs(cosHI * cosHO) * common_;
  pdf = G1o * abs(cosHO * cosHI) * common_;

  return vec3(out_);
}

Spectrum bsdf_microfacet_ggx_eval(const MicrofacetBsdf bsdf,
                                  const float3 I,
                                  const float3 omega_in,
                                  inout float pdf) {
  const float alpha_x = bsdf.alpha_x;
  const float alpha_y = bsdf.alpha_y;
  const bool m_refractive =
      bsdf.type == CLOSURE_BSDF_MICROFACET_GGX_REFRACTION_ID;
  const float3 N = bsdf.N;
  const float cosNO = dot(N, I);
  const float cosNI = dot(N, omega_in);

  if (((cosNI < 0.0f) != m_refractive) || alpha_x * alpha_y <= 1e-7f) {
    pdf = 0.0f;
    return vec3(0.0);
  }

  return (cosNI < 0.0f)
             ? bsdf_microfacet_ggx_eval_transmit(bsdf, N, I, omega_in, pdf,
                                                 alpha_x, alpha_y, cosNO, cosNI)
             : bsdf_microfacet_ggx_eval_reflect(bsdf, N, I, omega_in, pdf,
                                                alpha_x, alpha_y, cosNO, cosNI);
}

int bsdf_microfacet_ggx_sample(const MicrofacetBsdf bsdf,
                               float3 Ng,
                               float3 I,
                               float randu,
                               float randv,
                               inout Spectrum eval,
                               inout float3 omega_in,
                               inout float pdf,
                               inout float2 sampled_roughness,
                               inout float eta) {
  float alpha_x = bsdf.alpha_x;
  float alpha_y = bsdf.alpha_y;
  bool m_refractive = bsdf.type == CLOSURE_BSDF_MICROFACET_GGX_REFRACTION_ID;

  sampled_roughness = vec2(alpha_x, alpha_y);
  eta = m_refractive ? 1.0f / bsdf.ior : bsdf.ior;

  float3 N = bsdf.N;
  int label;

  float cosNO = dot(N, I);
  if (cosNO > 0) {
    float3 X, Y, Z = N;

    if (alpha_x == alpha_y)
      MakeOrthonormals(Z, X, Y);
    else
      make_orthonormals_tangent(Z, bsdf.T, X, Y);

    /* importance sampling with distribution of visible normals. vectors are
     * transformed to local space before and after */
    float3 local_I = vec3(dot(X, I), dot(Y, I), cosNO);
    float3 local_m;
    float G1o;

    local_m = microfacet_sample_stretched(local_I, alpha_x, alpha_y, randu,
                                          randv, false, G1o);

    float3 m = X * local_m.x + Y * local_m.y + Z * local_m.z;
    float cosThetaM = local_m.z;

    /* reflection or refraction? */
    if (!m_refractive) {
      float cosMO = dot(m, I);
      label = LABEL_REFLECT | LABEL_GLOSSY;

      if (cosMO > 0) {
        /* eq. 39 - compute actual reflected direction */
        omega_in = 2 * cosMO * m - I;

        if (dot(Ng, omega_in) > 0) {
          if (alpha_x * alpha_y <= 1e-7f) {
            /* some high number for MIS */
            pdf = 1e6f;
            eval = vec3(1e6f);

            bool use_fresnel =
                (bsdf.type == CLOSURE_BSDF_MICROFACET_GGX_FRESNEL_ID ||
                 bsdf.type == CLOSURE_BSDF_MICROFACET_GGX_CLEARCOAT_ID);

            /* if fresnel is used, calculate the color with
             * reflection_color(...) */
            if (use_fresnel) {
              eval *= reflection_color(bsdf, omega_in, m);
            }

            label = LABEL_REFLECT | LABEL_SINGULAR;
          } else {
            /* microfacet normal is visible to this ray */
            /* eq. 33 */
            float alpha2 = alpha_x * alpha_y;
            float D, G1i;

            if (alpha_x == alpha_y) {
              /* isotropic */
              float cosThetaM2 = cosThetaM * cosThetaM;
              float cosThetaM4 = cosThetaM2 * cosThetaM2;
              float tanThetaM2 = 1 / (cosThetaM2)-1;

              /* eval BRDF*cosNI */
              float cosNI = dot(N, omega_in);

              if (bsdf.type == CLOSURE_BSDF_MICROFACET_GGX_CLEARCOAT_ID) {
                /* use GTR1 for clearcoat */
                D = D_GTR1(cosThetaM, bsdf.alpha_x);

                /* the alpha value for clearcoat is a fixed 0.25 => alpha2 =
                 * 0.25 * 0.25 */
                alpha2 = 0.0625f;

                /* recalculate G1o */
                G1o = 2 / (1 + safe_sqrtf(1 + alpha2 * (1 - cosNO * cosNO) /
                                                  (cosNO * cosNO)));
              } else {
                /* use GTR2 otherwise */
                D = alpha2 / (PI * cosThetaM4 * (alpha2 + tanThetaM2) *
                              (alpha2 + tanThetaM2));
              }

              /* eq. 34: now calculate G1(i,m) */
              G1i = 2 / (1 + safe_sqrtf(1 + alpha2 * (1 - cosNI * cosNI) /
                                                (cosNI * cosNI)));
            } else {
              /* anisotropic distribution */
              float3 local_m = vec3(dot(X, m), dot(Y, m), dot(Z, m));
              float slope_x = -local_m.x / (local_m.z * alpha_x);
              float slope_y = -local_m.y / (local_m.z * alpha_y);
              float slope_len = 1 + slope_x * slope_x + slope_y * slope_y;

              float cosThetaM = local_m.z;
              float cosThetaM2 = cosThetaM * cosThetaM;
              float cosThetaM4 = cosThetaM2 * cosThetaM2;

              D = 1 / ((slope_len * slope_len) * PI * alpha2 * cosThetaM4);

              /* calculate G1(i,m) */
              float cosNI = dot(N, omega_in);

              float tanThetaI2 = (1 - cosNI * cosNI) / (cosNI * cosNI);
              float cosPhiI = dot(omega_in, X);
              float sinPhiI = dot(omega_in, Y);

              float alphaI2 = (cosPhiI * cosPhiI) * (alpha_x * alpha_x) +
                              (sinPhiI * sinPhiI) * (alpha_y * alpha_y);
              alphaI2 /= cosPhiI * cosPhiI + sinPhiI * sinPhiI;

              G1i = 2 / (1 + safe_sqrtf(1 + alphaI2 * tanThetaI2));
            }

            /* see eval function for derivation */
            float common_ = (G1o * D) * 0.25f / cosNO;
            pdf = common_;

            Spectrum F = reflection_color(bsdf, omega_in, m);

            eval = G1i * common_ * F;
          }

          if (bsdf.type == CLOSURE_BSDF_MICROFACET_GGX_CLEARCOAT_ID) {
            eval *= 0.25f * bsdf.clearcoat;
          }
        } else {
          eval = vec3(0);
          pdf = 0.0f;
        }
      }
    } else {
      label = LABEL_TRANSMIT | LABEL_GLOSSY;

      /* CAUTION: the i and o variables are inverted relative to the paper
       * eq. 39 - compute actual refractive direction */
      float3 R, T;
      float m_eta = bsdf.ior, fresnel;
      bool inside;

      fresnel = fresnel_dielectric(m_eta, m, I, R, T, inside);

      if (!inside && fresnel != 1.0f) {
        omega_in = T;

        if (alpha_x * alpha_y <= 1e-7f || abs(m_eta - 1.0f) < 1e-4f) {
          /* some high number for MIS */
          pdf = 1e6f;
          eval = vec3(1e6f);
          label = LABEL_TRANSMIT | LABEL_SINGULAR;
        } else {
          /* eq. 33 */
          float alpha2 = alpha_x * alpha_y;
          float cosThetaM2 = cosThetaM * cosThetaM;
          float cosThetaM4 = cosThetaM2 * cosThetaM2;
          float tanThetaM2 = 1 / (cosThetaM2)-1;
          float D = alpha2 / (PI * cosThetaM4 * (alpha2 + tanThetaM2) *
                              (alpha2 + tanThetaM2));

          /* eval BRDF*cosNI */
          float cosNI = dot(N, omega_in);

          /* eq. 34: now calculate G1(i,m) */
          float G1i = 2 / (1 + safe_sqrtf(1 + alpha2 * (1 - cosNI * cosNI) /
                                                  (cosNI * cosNI)));

          /* eq. 21 */
          float cosHI = dot(m, omega_in);
          float cosHO = dot(m, I);
          float Ht2 = m_eta * cosHI + cosHO;
          Ht2 *= Ht2;

          /* see eval function for derivation */
          float common_ = (G1o * D) * (m_eta * m_eta) / (cosNO * Ht2);
          float out_ = G1i * abs(cosHI * cosHO) * common_;
          pdf = cosHO * abs(cosHI) * common_;

          eval = vec3(out_);
        }
      } else {
        eval = vec3(0);
        pdf = 0.0f;
      }
    }
  } else {
    label = (m_refractive) ? LABEL_TRANSMIT | LABEL_GLOSSY
                           : LABEL_REFLECT | LABEL_GLOSSY;
  }
  return label;
}

#endif
