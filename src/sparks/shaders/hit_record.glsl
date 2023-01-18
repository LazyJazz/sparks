#ifndef HIT_RECORD_GLSL
#define HIT_RECORD_GLSL

#include "material.glsl"
#include "sample_texture.glsl"
#include "vertex.glsl"

struct HitRecord {
  int hit_entity_id;
  vec3 position;
  vec3 normal;
  vec3 shading_normal;
  vec3 geometry_normal;
  vec3 tangent;
  vec3 bitangent;
  vec2 tex_coord;
  bool front_face;
  vec3 omega_v;
  vec3 base_color;

  vec3 subsurface_color;
  float subsurface;

  vec3 subsurface_radius;
  float metallic;

  float specular;
  float specular_tint;
  float roughness;
  float anisotropic;

  float anisotropic_rotation;
  float sheen;
  float sheen_tint;
  float clearcoat;

  float clearcoat_roughness;
  float ior;
  float transmission;
  float transmission_roughness;

  vec3 emission;
  float emission_strength;
  float alpha;
  uint material_type;
};

HitRecord GetHitRecord(RayPayload ray_payload, vec3 origin, vec3 direction) {
  HitRecord hit_record;
  hit_record.hit_entity_id = 0;
  hit_record.position = vec3(0);
  hit_record.normal = vec3(0);
  hit_record.shading_normal = vec3(0);
  hit_record.geometry_normal = vec3(0);
  hit_record.tangent = vec3(0);
  hit_record.bitangent = vec3(0);
  hit_record.tex_coord = vec2(0);
  hit_record.front_face = true;
  hit_record.base_color = vec3(0);
  hit_record.omega_v = -direction;

  hit_record.subsurface_color = vec3(1.0);
  hit_record.subsurface = 0.0;
  hit_record.subsurface_radius = vec3(1.0);
  hit_record.metallic = 0.0;
  hit_record.specular = 0.0;
  hit_record.specular_tint = 0.0;
  hit_record.roughness = 0.0;
  hit_record.anisotropic = 0.0;
  hit_record.anisotropic_rotation = 0.0;
  hit_record.sheen = 0.0;
  hit_record.sheen_tint = 0.0;
  hit_record.clearcoat = 0.0;
  hit_record.clearcoat_roughness = 0.0;
  hit_record.ior = 1.0;
  hit_record.transmission = 0.0;
  hit_record.transmission_roughness = 0.0;

  hit_record.emission = vec3(0);
  hit_record.emission_strength = 0.0;
  hit_record.alpha = 1.0;
  hit_record.material_type = 0;

  ObjectInfo object_info = object_infos[ray_payload.object_id];
  Vertex v0 = GetVertex(
      object_info.vertex_offset +
      indices[object_info.index_offset + ray_payload.primitive_id * 3 + 0]);
  Vertex v1 = GetVertex(
      object_info.vertex_offset +
      indices[object_info.index_offset + ray_payload.primitive_id * 3 + 1]);
  Vertex v2 = GetVertex(
      object_info.vertex_offset +
      indices[object_info.index_offset + ray_payload.primitive_id * 3 + 2]);
  vec3 b0 = v0.signal * cross(v0.normal, v0.tangent);
  vec3 b1 = v1.signal * cross(v1.normal, v1.tangent);
  vec3 b2 = v2.signal * cross(v2.normal, v2.tangent);
  hit_record.hit_entity_id = int(ray_payload.object_id);

  mat3 object_to_world = mat3(ray_payload.object_to_world);
  hit_record.position = ray_payload.object_to_world *
                        vec4(mat3(v0.position, v1.position, v2.position) *
                                 ray_payload.barycentric,
                             1.0);
  hit_record.shading_normal = normalize(transpose(inverse(object_to_world)) *
                                        mat3(v0.normal, v1.normal, v2.normal) *
                                        ray_payload.barycentric);
  hit_record.geometry_normal =
      normalize(transpose(inverse(object_to_world)) *
                cross(v1.position - v0.position, v2.position - v0.position));
  hit_record.tangent =
      normalize(object_to_world * mat3(v0.tangent, v1.tangent, v2.tangent) *
                ray_payload.barycentric);
  hit_record.bitangent =
      normalize(object_to_world * mat3(b0, b1, b2) * ray_payload.barycentric);
  hit_record.tex_coord = mat3x2(v0.tex_coord, v1.tex_coord, v2.tex_coord) *
                         ray_payload.barycentric;

  if (dot(hit_record.geometry_normal, hit_record.shading_normal) < 0.0) {
    hit_record.geometry_normal = -hit_record.geometry_normal;
  }

  Material mat = materials[hit_record.hit_entity_id];
  hit_record.base_color =
      mat.base_color *
      SampleTexture(mat.base_color_texture_id, hit_record.tex_coord).xyz;

  hit_record.subsurface_color = mat.subsurface_color;
  hit_record.subsurface = mat.subsurface;
  hit_record.subsurface_radius = mat.subsurface_radius;
  hit_record.metallic = mat.metallic;
  hit_record.specular = mat.specular;
  hit_record.specular_tint = mat.specular_tint;
  hit_record.roughness = mat.roughness;
  hit_record.anisotropic = mat.anisotropic;
  hit_record.anisotropic_rotation = mat.anisotropic_rotation;
  hit_record.sheen = mat.sheen;
  hit_record.sheen_tint = mat.sheen_tint;
  hit_record.clearcoat = mat.clearcoat;
  hit_record.clearcoat_roughness = mat.clearcoat_roughness;
  hit_record.ior = mat.ior;
  hit_record.transmission = mat.transmission;
  hit_record.transmission_roughness = mat.transmission_roughness;

  hit_record.emission = mat.emission;
  hit_record.emission_strength = mat.emission_strength;

  hit_record.alpha = mat.alpha;
  hit_record.material_type = mat.material_type;

  vec3 relative_normal = vec3(0, 0, 1);
  float bitagent_signal = 1.0;
  if (mat.normal_map_id != -1) {
    if (mat.normal_map_id < 0) {
      bitagent_signal = -1.0;
    }
    mat.normal_map_id &= 0x3fffffff;
    relative_normal =
        (SampleTexture(mat.normal_map_id, hit_record.tex_coord).xyz - 0.5) *
        2.0;
    relative_normal = vec3(
        relative_normal.xy * mat.normal_map_intensity / relative_normal.z, 1.0);
    relative_normal = normalize(relative_normal);
  }
  hit_record.normal =
      mat3(hit_record.tangent, bitagent_signal * hit_record.bitangent,
           hit_record.shading_normal) *
      relative_normal;

  hit_record.front_face = true;
  if (dot(direction, hit_record.geometry_normal) > 0.0) {
    hit_record.front_face = false;
    hit_record.geometry_normal = -hit_record.geometry_normal;
    hit_record.shading_normal = -hit_record.shading_normal;
    hit_record.normal = -hit_record.normal;
    hit_record.tangent = -hit_record.tangent;
    hit_record.bitangent = -hit_record.bitangent;
  }

  return hit_record;
}

HitRecord hit_record;
#endif
