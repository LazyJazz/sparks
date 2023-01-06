struct HitRecord {
  int hit_entity_id;
  vec3 position;
  vec3 normal;
  vec3 geometry_normal;
  vec3 tangent;
  vec2 tex_coord;
  bool front_face;
};

HitRecord GetHitRecord(RayPayload ray_payload, vec3 origin, vec3 direction) {
  HitRecord hit_record;
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
  hit_record.hit_entity_id = int(ray_payload.object_id);

  mat3 object_to_world = mat3(ray_payload.object_to_world);
  hit_record.position = ray_payload.object_to_world *
                        vec4(mat3(v0.position, v1.position, v2.position) *
                                 ray_payload.barycentric,
                             1.0);
  hit_record.normal = normalize(transpose(inverse(object_to_world)) *
                                mat3(v0.normal, v1.normal, v2.normal) *
                                ray_payload.barycentric);
  hit_record.geometry_normal =
      normalize(transpose(inverse(object_to_world)) *
                cross(v1.position - v0.position, v2.position - v0.position));
  hit_record.tangent =
      normalize(object_to_world * mat3(v0.tangent, v1.tangent, v2.tangent) *
                ray_payload.barycentric);
  hit_record.tex_coord = v0.tex_coord * ray_payload.barycentric.x +
                         v1.tex_coord * ray_payload.barycentric.y +
                         v2.tex_coord * ray_payload.barycentric.z;

  if (dot(hit_record.geometry_normal, hit_record.normal) < 0.0) {
    hit_record.geometry_normal = -hit_record.geometry_normal;
  }

  hit_record.front_face = true;
  if (dot(direction, hit_record.geometry_normal) > 0.0) {
    hit_record.front_face = false;
    hit_record.geometry_normal = -hit_record.geometry_normal;
    hit_record.normal = -hit_record.normal;
    hit_record.tangent = -hit_record.tangent;
  }

  return hit_record;
}
