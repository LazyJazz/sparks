#include "sparks/assets/camera.h"

#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"

namespace sparks {

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
  return glm::scale(glm::mat4{1.0f}, glm::vec3{1.0f, -1.0f, 1.0f}) *
         glm::perspectiveZO(glm::radians(fov_), aspect, 0.1f, 1000.0f);
}

void Camera::ImGuiItems() {
  ImGui::SliderFloat("FOV", &fov_, 10.0f, 160.0f, "%.0f", 0);
}

void Camera::UpdateFov(float delta) {
  fov_ += delta;
  fov_ = glm::clamp(fov_, 10.0f, 160.0f);
}
}  // namespace sparks
