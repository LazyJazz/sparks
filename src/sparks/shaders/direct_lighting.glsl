#ifndef DIRECT_LIGHTING_GLSL
#define DIRECT_LIGHTING_GLSL
#include "envmap.glsl"
#include "random.glsl"
#include "trace_ray.glsl"
#include "vertex.glsl"

float EstimateDirectLightingPdf() {
  float pdf = 0.0;
  float model_light_weight = 0.0;
  float envmap_light_weight = 0.0;
  float total_light_weight = 0.0;
  if (global_uniform_object.total_power > 1e-4) {
    model_light_weight = 1.0;
  }
  //  if (global_uniform_object.total_envmap_power > 1e-4) {
  //    envmap_light_weight = 1.0;
  //  }
  total_light_weight = model_light_weight + envmap_light_weight;
  model_light_weight /= total_light_weight;
  envmap_light_weight /= total_light_weight;
  if (model_light_weight > 0.0) {
    if (ray_payload.t != -1.0) {
      pdf += object_sampler_infos[ray_payload.object_id].sample_density *
             ray_payload.t * ray_payload.t * model_light_weight;
    }
  }
  if (envmap_light_weight > 0.0) {
    if (ray_payload.t == -1.0) {
      vec3 color = SampleEnvmap(trace_ray_direction);
      float strength = max(color.r, max(color.g, color.b));
      pdf += envmap_light_weight * strength /
             global_uniform_object.total_envmap_power;
    }
  }
  return pdf;
}

float EvalDirectLighting(vec3 omega_in) {
  if (global_uniform_object.total_power > 1e-4 ||
      global_uniform_object.total_envmap_power > 1e-4) {
    TraceRay(hit_record.position, omega_in);
    return EstimateDirectLightingPdf();
  }
  return 0.0;
}

void SampleModelLighting(out vec3 eval,
                         out vec3 omega_in,
                         out float pdf,
                         float r1) {
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
  primitive_index--;
  float lbound = primitive_cdf[object_sampler_info.primitive_offset +
                               primitive_index],
        rbound = primitive_cdf[object_sampler_info.primitive_offset +
                               primitive_index + 1];
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
  vec3 geometry_normal =
      normalize(transpose(inverse(object_to_world)) * cross(v1 - v0, v2 - v0));
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
    eval = dot(-omega_in, geometry_normal) * global_uniform_object.total_power *
           materials[object_index].emission / (dist * dist);
  }
}

void SampleEnvmapLighting(out vec3 eval,
                          out vec3 omega_in,
                          out float pdf,
                          float r1) {
  ivec2 envmap_size =
      textureSize(texture_samplers[global_uniform_object.envmap_id], 0);
  int L = 0, R = envmap_size.x * envmap_size.y;
  while (L < R) {
    int m = (L + R) / 2;
    if (r1 <= envmap_cdf[m]) {
      R = m;
    } else {
      L = m + 1;
    }
  }
  if (L == envmap_size.x * envmap_size.y) {
    return;
  }
  int x, y = L / envmap_size.x;
  x = L - y * envmap_size.x;
  float inv_width = 1.0 / float(envmap_size.x);
  float inv_height = 1.0 / float(envmap_size.y);
  float z_lbound = cos(y * inv_height * PI);
  float z_rbound = cos((y + 1) * inv_height * PI);
  vec2 tex_coord = vec2((x + RandomFloat()) * inv_width,
                        acos(r1 * (z_rbound - z_lbound) + z_lbound) * INV_PI);
  omega_in = EnvmapCoordToDirection(tex_coord);
  pdf = EvalDirectLighting(omega_in);
  if (ray_payload.t == -1.0) {
    eval = SampleEnvmapTexCoord(tex_coord) * 4 * PI / pdf;
  }
}

void SampleDirectLighting(out vec3 eval, out vec3 omega_in, out float pdf) {
  eval = vec3(0.0);
  omega_in = vec3(0.0);
  pdf = 0.0;
  float model_light_weight = 0.0;
  float envmap_light_weight = 0.0;
  float total_light_weight = 0.0;
  if (global_uniform_object.total_power > 1e-4) {
    model_light_weight = 1.0;
  }
  //  if (global_uniform_object.total_envmap_power > 1e-4) {
  //    envmap_light_weight = 1.0;
  //  }
  total_light_weight = model_light_weight + envmap_light_weight;
  if (total_light_weight < 1e-4) {
    return;
  }
  model_light_weight /= total_light_weight;
  envmap_light_weight /= total_light_weight;

  float r1 = RandomFloat();
  if (r1 < envmap_light_weight) {
    r1 /= envmap_light_weight;
    SampleEnvmapLighting(eval, omega_in, pdf, r1);
    eval /= envmap_light_weight;
  } else {
    r1 -= envmap_light_weight;
    r1 /= model_light_weight;
    SampleModelLighting(eval, omega_in, pdf, r1);
    // eval /= model_light_weight;
  }
}

#endif
