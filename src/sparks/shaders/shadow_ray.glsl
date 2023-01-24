#ifndef SHADOW_RAY_GLSL
#define SHADOW_RAY_GLSL

float ShadowRay(vec3 origin, vec3 direction, float dist) {
  float result = 1.0f;
  rayQueryEXT rq;
  if (global_uniform_object.enable_alpha_shadow) {
    rayQueryInitializeEXT(rq, scene, gl_RayFlagsNoOpaqueEXT, 0xFF, origin,
                          length(origin) * 1e-3, direction, dist * 0.999);

    // Traverse the acceleration structure and store information about the first
    // intersection (if any)
    while (rayQueryProceedEXT(rq)) {
      uint type = rayQueryGetIntersectionTypeEXT(rq, false);
      if (type == gl_RayQueryCandidateIntersectionTriangleEXT) {
        float alpha =
            materials[rayQueryGetIntersectionInstanceCustomIndexEXT(rq, false)]
                .alpha;
        result *= 1.0 - alpha;
      } else
        return 0.0;
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
  return result;
}

#endif
