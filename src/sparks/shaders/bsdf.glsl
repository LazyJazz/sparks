#define Ng hit_record.geometry_normal
#define N hit_record.normal
#define V hit_record.omega_v
#define I hit_record.omega_v
#define omega_in L

vec3 EvalSpecularBSDF(in vec3 L, out float pdf) {
  pdf = 0.0;
  return vec3(0.0);
}

vec3 EvalLambertianBSDF(in vec3 L, out float pdf) {
  float cos_pi = max(dot(N, omega_in), 0.0f) * INV_PI;
  pdf = cos_pi;
  return vec3(cos_pi) * hit_record.base_color;
}

vec3 EvalBSDF(in vec3 L, out float pdf) {
  switch (hit_record.material_type) {
    case MATERIAL_TYPE_SPECULAR:
      return EvalSpecularBSDF(L, pdf);
    case MATERIAL_TYPE_LAMBERTIAN:
      return EvalLambertianBSDF(L, pdf);
    default:
      pdf = 0.0f;
      return vec3(0);
  }
}

void SampleSpecularBSDF(out vec3 eval, out vec3 L, out float pdf) {
  float cosNO = dot(N, I);
  if (cosNO > 0) {
    omega_in = (2 * cosNO) * N - I;
    if (dot(Ng, omega_in) > 0) {
      pdf = 1e6;
      eval = vec3(1e6);
    }
  } else {
    pdf = 0.0f;
    eval = vec3(0.0);
  }
}

void SampleLambertianBSDF(out vec3 eval, out vec3 L, out float pdf) {
  SampleCosHemisphere(N, L, pdf);
  if (dot(Ng, L) > 0.0) {
    eval = vec3(pdf) * hit_record.base_color;
  } else {
    pdf = 0.0f;
    eval = vec3(0.0);
  }
}

void SampleBSDF(out vec3 eval, out vec3 L, out float pdf) {
  switch (hit_record.material_type) {
    case MATERIAL_TYPE_SPECULAR:
      SampleSpecularBSDF(eval, L, pdf);
      return;
    case MATERIAL_TYPE_LAMBERTIAN:
      SampleLambertianBSDF(eval, L, pdf);
      return;
    default:
      pdf = 0.0;
      eval = vec3(0.0f);
      return;
  }
}
