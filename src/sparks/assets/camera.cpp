#include "sparks/assets/camera.h"

#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"

namespace sparks {

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
  return glm::perspectiveLH_ZO(glm::radians(fov_), aspect, 0.1f, 10.0f);
}

void Camera::ImGuiItems() {
  ImGui::SliderFloat("FOV", &fov_, 10.0f, 160.0f, "%.0f", 0);
}
}  // namespace sparks
