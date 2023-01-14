#ifndef MIS_GLSL
#define MIS_GLSL

float PowerHeuristic(float base, float ref) {
  return (base * base) / (base * base + ref * ref);
}

#endif
