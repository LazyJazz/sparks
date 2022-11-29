#pragma once
#include "glm/glm.hpp"
#include "iostream"
#include "sparks/assets/aabb.h"

namespace sparks {
class Model {
 public:
  virtual ~Model() = default;
  [[nodiscard]] virtual float TraceRay(const glm::vec3 &origin,
                                       const glm::vec3 &direction,
                                       float t_min,
                                       float) const = 0;
  [[nodiscard]] virtual AxisAlignedBoundingBox GetAABB(
      const glm::mat4 &transform) const = 0;
};
}  // namespace sparks
