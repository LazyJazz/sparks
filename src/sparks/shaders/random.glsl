#ifndef RANDOM_GLSL
#define RANDOM_GLSL
struct RandomDevice {
  uint offset;
  uint samp;
  uint seed;
  uint dim;
} random_device;

uint GrayCode(uint i) {
  return i ^ (i >> 1);
}

uint WangHash(inout uint seed) {
  seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
  seed *= uint(9);
  seed = seed ^ (seed >> 4);
  seed *= uint(0x27d4eb2d);
  seed = seed ^ (seed >> 15);
  return seed;
}

uint WangHashS(uint seed) {
  seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
  seed *= uint(9);
  seed = seed ^ (seed >> 4);
  seed *= uint(0x27d4eb2d);
  seed = seed ^ (seed >> 15);
  return seed;
}

void InitRandomSeed(uint x, uint y, uint s) {
  random_device.offset = WangHashS(WangHashS(x) ^ y);
  random_device.seed = WangHashS(random_device.offset ^ s);
  random_device.dim = 0;
  random_device.samp = GrayCode(s);
}

uint SobolUint() {
  uint result = sobol_table[random_device.samp * 1024 + (random_device.dim)] ^
                WangHash(random_device.offset);
  random_device.dim++;
  return result;
}

uint RandomUint() {
  if (random_device.dim < 1024 && random_device.samp < 65536)
    return SobolUint();
  return WangHash(random_device.seed);
}

float RandomFloat() {
  return float(RandomUint()) / 4294967296.0;
}

vec2 RandomOnCircle() {
  float theta = RandomFloat() * PI * 2.0;
  return vec2(sin(theta), cos(theta));
}

vec2 RandomInCircle() {
  return RandomOnCircle() * sqrt(RandomFloat());
}

vec3 RandomOnSphere() {
  float theta = RandomFloat() * PI * 2.0;
  float z = RandomFloat() * 2.0 - 1.0;
  float xy = sqrt(1.0 - z * z);
  return vec3(xy * RandomOnCircle(), z);
}

vec3 RandomInSphere() {
  return RandomOnSphere() * pow(RandomFloat(), 0.3333333333333333333);
}

void MakeOrthonormals(const vec3 N, out vec3 a, out vec3 b) {
  if (N.x != N.y || N.x != N.z)
    a = vec3(N.z - N.y, N.x - N.z, N.y - N.x);
  else
    a = vec3(N.z - N.y, N.x + N.z, -N.y - N.x);

  a = normalize(a);
  b = cross(N, a);
}

void sample_cos_hemisphere(const vec3 N,
                           float r1,
                           const float r2,
                           out vec3 omega_in,
                           out float pdf) {
  r1 *= PI * 2.0;
  vec3 T, B;
  MakeOrthonormals(N, T, B);
  omega_in = vec3(vec2(sin(r1), cos(r1)) * sqrt(1.0 - r2), sqrt(r2));
  pdf = omega_in.z * INV_PI;
  omega_in = mat3(T, B, N) * omega_in;
}

void SampleCosHemisphere(const vec3 N, out vec3 omega_in, out float pdf) {
  sample_cos_hemisphere(N, RandomFloat(), RandomFloat(), omega_in, pdf);
}
#endif
