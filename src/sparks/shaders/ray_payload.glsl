
struct RayPayload {
  mat4x3 object_to_world;
  vec3 barycentric;
  float t;
  uint object_id;
  uint primitive_id;
};
