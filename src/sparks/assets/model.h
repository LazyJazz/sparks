#pragma once
#include "glm/glm.hpp"
#include "iostream"
#include "sparks/assets/aabb.h"
#include "sparks/assets/hit_record.h"
#include "sparks/assets/vertex.h"
#include "sparks/util/util.h"
#include "vector"

namespace sparks {
class Model {
 public:
  virtual ~Model() = default;
  [[nodiscard]] virtual float TraceRay(const glm::vec3 &origin,
                                       const glm::vec3 &direction,
                                       float t_min,
                                       HitRecord *hit_record) const = 0;
  [[nodiscard]] virtual AxisAlignedBoundingBox GetAABB(
      const glm::mat4 &transform) const = 0;
  [[nodiscard]] virtual std::vector<Vertex> GetVertices() const = 0;
  [[nodiscard]] virtual std::vector<uint32_t> GetIndices() const = 0;
  virtual const char *GetDefaultEntityName();
};
}  // namespace sparks
