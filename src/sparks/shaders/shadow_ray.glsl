#ifndef SHADOW_RAY_GLSL
#define SHADOW_RAY_GLSL

float ShadowRay(vec3 origin, vec3 direction, float dist) {
  float result = 1.0f;
  rayQueryEXT rq;

  rayQueryInitializeEXT(rq, scene, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF,
                        origin, length(origin) * 1e-3, direction, dist * 0.999);

  // Traverse the acceleration structure and store information about the first
  // intersection (if any)
  rayQueryProceedEXT(rq);

  if (rayQueryGetIntersectionTypeEXT(rq, true) !=
      gl_RayQueryCommittedIntersectionNoneEXT) {
    result = 0.0f;
  }
  return result;
}

#endif
