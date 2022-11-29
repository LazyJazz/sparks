#pragma once
#include "sparks/assets/model.h"
#include "sparks/assets/vertex.h"
#include "vector"

namespace sparks {
class Mesh : public Model {
 public:
  Mesh(const std::vector<Vertex> &vertices,
       const std::vector<uint32_t> &indices);
  float TraceRay(const glm::vec3 &origin,
                 const glm::vec3 &direction,
                 float t_min,
                 float) override;
  [[nodiscard]] AxisAlignedBoundingBox GetAABB(
      const glm::mat4 &transform) const override;
  static Mesh Cube(const glm::vec3 &center, const glm::vec3 &size);
  static Mesh Sphere(const glm::vec3 &center = glm::vec3{0.0f},
                     float radius = 1.0f);
  void WriteObjFile(const std::string &file_path) const;

 private:
  std::vector<Vertex> vertices_;
  std::vector<uint32_t> indices_;
};
}  // namespace sparks
