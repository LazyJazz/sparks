#pragma once
#include "glm/glm.hpp"

namespace sparks {
class Camera {
 public:
  [[nodiscard]] glm::mat4 GetProjectionMatrix(float aspect) const;
  void ImGuiItems();

 private:
  float fov_{60.0f};
};
}  // namespace sparks
