#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : enable
// clang-format off
#include "ray_payload.glsl"
// clang-format on

layout(location = 0) rayPayloadInEXT RayPayload ray_payload;
hitAttributeEXT vec3 attribs;

void main() {
  ray_payload.t = gl_HitTEXT;
  ray_payload.barycentric =
      vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
  ray_payload.object_id = gl_InstanceCustomIndexEXT;
  ray_payload.primitive_id = gl_PrimitiveID;
}
