#ifndef TRACE_RAY_GLSL
#define TRACE_RAY_GLSL

vec3 trace_ray_direction;

void TraceRay(vec3 origin, vec3 direction) {
  trace_ray_direction = direction;
  float tmin = 1e-4 * length(origin);
  float tmax = 1e4;

  ray_payload.t = -1.0;
  ray_payload.barycentric = vec3(0.0);
  ray_payload.object_id = 0;
  ray_payload.primitive_id = 0;
  ray_payload.object_to_world = mat4x3(1.0);

  traceRayEXT(scene, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin,
              direction, tmax, 0);
}

#endif
