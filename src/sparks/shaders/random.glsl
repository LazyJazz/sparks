
struct RandomDevice {
  uint seed;
} random_device;

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
  random_device.seed = WangHashS(WangHashS(WangHashS(x) ^ y) ^ s);
}

float RandomFloat() {
  return float(WangHash(random_device.seed)) / 4294967296.0;
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
