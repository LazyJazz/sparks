#include "sparks/assets/accelerated_mesh.h"

#include "algorithm"

namespace sparks {
AcceleratedMesh::AcceleratedMesh(const Mesh &mesh) : Mesh(mesh) {
  BuildAccelerationStructure();
}

AcceleratedMesh::AcceleratedMesh(const std::vector<Vertex> &vertices,
                                 const std::vector<uint32_t> &indices)
    : Mesh(vertices, indices) {
  BuildAccelerationStructure();
  printf("%d\n", root_);
  for (int i = 0; i < tree_node_.size(); i++) {
    auto &node = tree_node_[i];
    printf("(%f, %f, %f) (%f, %f, %f)\n", node.aabb.x_low, node.aabb.y_low,
           node.aabb.z_low, node.aabb.x_high, node.aabb.y_high,
           node.aabb.z_high);
    printf("%d %d\n", node.child[0], node.child[1]);
  }
}

void AcceleratedMesh::BuildAccelerationStructure() {
  std::vector<std::pair<int, glm::vec3>> triangle_indices;
  for (int i = 0; i * 3 + 2 < indices_.size(); i++) {
    triangle_indices.emplace_back(i, (vertices_[indices_[i]].position +
                                      vertices_[indices_[i + 1]].position +
                                      vertices_[indices_[i + 2]].position) /
                                         3.0f);
  }
  tree_node_.resize(triangle_indices.size());
  BuildTree(root_, triangle_indices.data(), 0, int(triangle_indices.size()), 0);
}

void AcceleratedMesh::BuildTree(int &x,
                                std::pair<int, glm::vec3> *triangle_list,
                                int L,
                                int R,
                                int cut) {
  if (L >= R) {
    x = -1;
    return;
  }
  auto mid = (L + R) >> 1;
  std::sort(triangle_list + L, triangle_list + R,
            [cut](const std::pair<int, glm::vec3> &p1,
                  const std::pair<int, glm::vec3> &p2) {
              return p1.second[cut] < p2.second[cut];
            });
  x = triangle_list[mid].first;
  tree_node_[x].aabb =
      AxisAlignedBoundingBox(vertices_[indices_[x * 3]].position) |
      AxisAlignedBoundingBox(vertices_[indices_[x * 3 + 1]].position) |
      AxisAlignedBoundingBox(vertices_[indices_[x * 3 + 2]].position);
  BuildTree(tree_node_[x].child[0], triangle_list, L, mid, (cut + 1) % 3);
  BuildTree(tree_node_[x].child[1], triangle_list, mid + 1, R, (cut + 1) % 3);
  if (tree_node_[x].child[0] != -1) {
    tree_node_[x].aabb |= tree_node_[tree_node_[x].child[0]].aabb;
  }
  if (tree_node_[x].child[1] != -1) {
    tree_node_[x].aabb |= tree_node_[tree_node_[x].child[1]].aabb;
  }
}

float AcceleratedMesh::TraceRay(const glm::vec3 &origin,
                                const glm::vec3 &direction,
                                float t_min,
                                HitRecord *hit_record) const {
  float t = -1.0f;
  TraceRay(root_, origin, direction, t_min, &t, hit_record);
  return t;
}

void AcceleratedMesh::TraceRay(int x,
                               const glm::vec3 &origin,
                               const glm::vec3 &direction,
                               float t_min,
                               float *result,
                               HitRecord *hit_record) const {
  if (x == -1) {
    return;
  }
  float t_max = *result;
  if (t_max < t_min) {
    t_max = 1e10;
  }
  if (!tree_node_[x].aabb.IsIntersect(origin, direction, t_min, t_max)) {
    return;
  }
  do {
    const auto &v0 = vertices_[indices_[x * 3]];
    const auto &v1 = vertices_[indices_[x * 3 + 1]];
    const auto &v2 = vertices_[indices_[x * 3 + 2]];

    glm::mat3 A = glm::mat3(v1.position - v0.position,
                            v2.position - v0.position, -direction);
    if (std::abs(glm::determinant(A)) < 1e-9f) {
      break;
    }
    A = glm::inverse(A);
    auto uvt = A * (origin - v0.position);
    auto &t = uvt.z;
    if (t < t_min || (*result > 0.0f && t > *result)) {
      break;
    }
    auto &u = uvt.x;
    auto &v = uvt.y;
    auto w = 1.0f - u - v;
    auto position = origin + t * direction;
    if (u >= 0.0f && v >= 0.0f && u + v <= 1.0f) {
      *result = t;
      if (hit_record) {
        auto geometry_normal = glm::normalize(
            glm::cross(v1.position - v0.position, v2.position - v0.position));
        if (glm::dot(geometry_normal, direction) < 0.0f) {
          hit_record->position = position;
          hit_record->geometry_normal = geometry_normal;
          hit_record->normal = v0.normal * w + v1.normal * u + v2.normal * v;
          hit_record->tangent =
              v0.tangent * w + v1.tangent * u + v2.tangent * v;
          hit_record->tex_coord =
              v0.tex_coord * w + v1.tex_coord * u + v2.tex_coord * v;
          hit_record->front_face = true;
        } else {
          hit_record->position = position;
          hit_record->geometry_normal = -geometry_normal;
          hit_record->normal = -(v0.normal * w + v1.normal * u + v2.normal * v);
          hit_record->tangent =
              -(v0.tangent * w + v1.tangent * u + v2.tangent * v);
          hit_record->tex_coord =
              v0.tex_coord * w + v1.tex_coord * u + v2.tex_coord * v;
          hit_record->front_face = false;
        }
      }
    }
  } while (false);
  TraceRay(tree_node_[x].child[0], origin, direction, t_min, result,
           hit_record);
  TraceRay(tree_node_[x].child[1], origin, direction, t_min, result,
           hit_record);
}

}  // namespace sparks
