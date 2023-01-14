#ifndef DIRECT_LIGHTING_GLSL
#define DIRECT_LIGHTING_GLSL
#include "random.glsl"
#include "vertex.glsl"

float EstimateDirectLightingPdf() {
  float pdf = 0.0;
  if (global_uniform_object.total_power > 1e-4) {
    if (ray_payload.t != -1.0) {
      pdf += object_sampler_infos[ray_payload.object_id].sample_density *
             ray_payload.t * ray_payload.t;
    }
  }
  return pdf;
}

float EvalDirectLighting(vec3 omega_in) {
  if (global_uniform_object.total_power > 1e-4) {
    ray_payload.t = -1.0;
    ray_payload.barycentric = vec3(0.0);
    ray_payload.object_id = 0;
    ray_payload.primitive_id = 0;
    ray_payload.object_to_world = mat4x3(1.0);

    traceRayEXT(scene, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, hit_record.position,
                1e-4 * length(hit_record.position), omega_in, 1e4, 0);
  }
  return EstimateDirectLightingPdf();
}

void SampleDirectLighting(out vec3 eval, out vec3 omega_in, out float pdf) {
  eval = vec3(0.0);
  omega_in = vec3(0.0);
  pdf = 0.0;
  float r1 = RandomFloat();
  if (global_uniform_object.total_power > 1e-4) {
    int L = 0, R = global_uniform_object.num_objects;
    while (L < R) {
      int m = (L + R) / 2;
      if (r1 <= object_sampler_infos[m].cdf) {
        R = m;
      } else {
        L = m + 1;
      }
    }
    int object_index = R;
    if (object_index >= global_uniform_object.num_objects) {
      return;
    }
    if (object_index > 0) {
      r1 -= object_sampler_infos[object_index - 1].cdf;
    }
    ObjectSamplerInfo object_sampler_info = object_sampler_infos[object_index];
    r1 /= object_sampler_info.pdf;
    L = 0;
    R = object_sampler_info.num_primitives + 1;
    while (L < R) {
      int m = (L + R) / 2;
      if (r1 <= primitive_cdf[object_sampler_info.primitive_offset + m]) {
        R = m;
      } else {
        L = m + 1;
      }
    }
    int primitive_index = L;
    if (primitive_index == 0 ||
        primitive_index == object_sampler_info.num_primitives + 1) {
      return;
    }
    float lbound = primitive_cdf[object_sampler_info.primitive_offset +
                                 primitive_index - 1],
          rbound = primitive_cdf[object_sampler_info.primitive_offset +
                                 primitive_index];
    r1 = (r1 - lbound) / (rbound - lbound);
    ObjectInfo object_info = object_infos[object_index];
    vec3 v0 = GetVertexPosition(
        object_info.vertex_offset +
        indices[object_info.index_offset + primitive_index * 3 + 0]);
    vec3 v1 = GetVertexPosition(
        object_info.vertex_offset +
        indices[object_info.index_offset + primitive_index * 3 + 1]);
    vec3 v2 = GetVertexPosition(
        object_info.vertex_offset +
        indices[object_info.index_offset + primitive_index * 3 + 2]);
    vec3 barycentric = vec3(r1, RandomFloat(), 0.0);
    if (barycentric.x + barycentric.y > 1.0) {
      barycentric.x = 1.0 - barycentric.x;
      barycentric.y = 1.0 - barycentric.y;
    }
    barycentric.z = 1.0 - barycentric.x - barycentric.y;

    mat3 object_to_world = mat3(object_sampler_info.object_to_world);
    vec3 position = vec3(object_sampler_info.object_to_world *
                         vec4(mat3(v0, v1, v2) * barycentric, 1.0));
    vec3 geometry_normal = normalize(transpose(inverse(object_to_world)) *
                                     cross(v1 - v0, v2 - v0));
    omega_in = position - hit_record.position;
    float dist = length(omega_in);
    if (dist < 1e-4) {
      return;
    }
    omega_in = normalize(omega_in);
    if (dot(geometry_normal, omega_in) > 0.0) {
      geometry_normal = -geometry_normal;
    }

    pdf = EvalDirectLighting(omega_in);

    if (abs(ray_payload.t - dist) < max(dist * 1e-4, 1e-4)) {
      eval = dot(-omega_in, geometry_normal) *
             global_uniform_object.total_power *
             materials[object_index].emission / (dist * dist);
    }
  }
}

#endif
