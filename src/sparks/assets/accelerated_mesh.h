#pragma once
#include "sparks/assets/aabb.h"
#include "sparks/assets/mesh.h"

namespace sparks {

namespace {
struct TreeNode {
  AxisAlignedBoundingBox aabb{};
  int child[2]{-1, -1};
};
}  // namespace

class AcceleratedMesh : public Mesh {
 public:
  explicit AcceleratedMesh(const Mesh &mesh);
  AcceleratedMesh(const std::vector<Vertex> &vertices,
                  const std::vector<uint32_t> &indices);
  float TraceRay(const glm::vec3 &origin,
                 const glm::vec3 &direction,
                 float t_min,
                 HitRecord *hit_record) const override;

 private:
  void BuildAccelerationStructure();
  void BuildTree(int &x,
                 std::pair<int, glm::vec3> *triangle_list,
                 int L,
                 int R,
                 int cut = 0);
  void TraceRay(int x,
                const glm::vec3 &origin,
                const glm::vec3 &direction,
                float t_min,
                float *t,
                HitRecord *hit_record) const;
  int root_{-1};
  std::vector<TreeNode> tree_node_;
};
}  // namespace sparks
