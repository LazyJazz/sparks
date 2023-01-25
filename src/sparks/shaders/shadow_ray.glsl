#ifndef SHADOW_RAY_GLSL
#define SHADOW_RAY_GLSL
#include "random.glsl"

float ShadowRay(vec3 origin, vec3 direction, float dist) {
  rayQueryEXT rq;
  if (global_uniform_object.enable_alpha_shadow) {
    float r1 = RandomFloat();
    rayQueryInitializeEXT(rq, scene, gl_RayFlagsNoOpaqueEXT, 0xFF, origin,
                          length(origin) * 1e-3, direction, dist * 0.999);

    // Traverse the acceleration structure and store information about the first
    // intersection (if any)
    while (rayQueryProceedEXT(rq)) {
      uint type = rayQueryGetIntersectionTypeEXT(rq, false);
      float alpha =
          materials[rayQueryGetIntersectionInstanceCustomIndexEXT(rq, false)]
              .alpha;
      if (r1 < 1.0 - alpha) {
        r1 /= 1.0 - alpha;
      } else {
        return 0.0;
      }
    }
  } else {
    rayQueryInitializeEXT(rq, scene, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF,
                          origin, length(origin) * 1e-3, direction,
                          dist * 0.999);
    rayQueryProceedEXT(rq);

    if (rayQueryGetIntersectionTypeEXT(rq, true) !=
        gl_RayQueryCommittedIntersectionNoneEXT) {
      return 0.0f;
    }
  }
  return 1.0;
}

#endif
