#include "sparks/assets/util.h"

#include "glm/gtc/matrix_transform.hpp"
#include "grassland/grassland.h"
#include "iostream"
#include "sstream"
#include "unordered_map"
#include "vector"

// clang-format off
#include "imgui.h"
#include "ImGuizmo.h"
// clang-format on

namespace sparks {
glm::vec3 DecomposeRotation(glm::mat3 R) {
  return {
      std::atan2(-R[2][1], std::sqrt(R[0][1] * R[0][1] + R[1][1] * R[1][1])),
      std::atan2(R[2][0], R[2][2]), std::atan2(R[0][1], R[1][1])};
}

glm::mat4 ComposeRotation(glm::vec3 pitch_yaw_roll) {
  return glm::rotate(glm::mat4{1.0f}, pitch_yaw_roll.y,
                     glm::vec3{0.0f, 1.0f, 0.0f}) *
         glm::rotate(glm::mat4{1.0f}, pitch_yaw_roll.x,
                     glm::vec3{1.0f, 0.0f, 0.0f}) *
         glm::rotate(glm::mat4{1.0f}, pitch_yaw_roll.z,
                     glm::vec3{0.0f, 0.0f, 1.0f});
}

glm::vec2 StringToVec2(const std::string &s) {
  std::istringstream ss(s);
  std::vector<float> v;
  std::string word;
  while (ss >> word)
    v.push_back(std::stof(word));
  return {v[0], v[1]};
}

glm::vec3 StringToVec3(const std::string &s) {
  std::istringstream ss(s);
  std::vector<float> v;
  std::string word;
  while (ss >> word)
    v.push_back(std::stof(word));
  return {v[0], v[1], v[2]};
}

glm::vec4 StringToVec4(const std::string &s) {
  std::istringstream ss(s);
  std::vector<float> v;
  std::string word;
  while (ss >> word)
    v.push_back(std::stof(word));
  return {v[0], v[1], v[2], v[3]};
}

glm::mat4 XmlTransformMatrix(tinyxml2::XMLElement *transform_element) {
  if (!transform_element)
    return glm::mat4{1.0f};
  std::string transform_type =
      transform_element->FindAttribute("type")->Value();
  if (transform_type == "lookat") {
    glm::vec3 eye{0.0f, 0.0f, 1.0f};
    glm::vec3 center{0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    auto child_element = transform_element->FirstChildElement("eye");
    if (child_element) {
      eye = StringToVec3(child_element->FindAttribute("value")->Value());
    }
    child_element = transform_element->FirstChildElement("center");
    if (child_element) {
      center = StringToVec3(child_element->FindAttribute("value")->Value());
    }
    child_element = transform_element->FirstChildElement("up");
    if (child_element) {
      up = StringToVec3(child_element->FindAttribute("value")->Value());
    }
    return glm::inverse(glm::lookAt(eye, center, up));
  } else if (transform_type == "translate") {
    glm::vec3 translation =
        StringToVec3(transform_element->FindAttribute("value")->Value());
    return glm::translate(glm::mat4{1.0f}, translation);
  } else if (transform_type == "rotate") {
    float angle = glm::radians(
        std::stof(transform_element->FindAttribute("angle")->Value()));
    glm::vec3 v =
        StringToVec3(transform_element->FindAttribute("axis")->Value());
    return glm::rotate(glm::mat4{1.0f}, angle, v);
  } else if (transform_type == "world") {
    glm::vec3 scale{1.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 translation(0.0f);
    auto child_element = transform_element->FirstChildElement("scale");
    if (child_element) {
      scale = StringToVec3(child_element->FindAttribute("value")->Value());
    }
    child_element = transform_element->FirstChildElement("rotation");
    if (child_element) {
      rotation = StringToVec3(child_element->FindAttribute("value")->Value());
    }
    child_element = transform_element->FirstChildElement("translation");
    if (child_element) {
      translation =
          StringToVec3(child_element->FindAttribute("value")->Value());
    }

    glm::mat4 matrix;
    ImGuizmo::RecomposeMatrixFromComponents(
        reinterpret_cast<float *>(&translation),
        reinterpret_cast<float *>(&rotation), reinterpret_cast<float *>(&scale),
        reinterpret_cast<float *>(&matrix));
    return matrix;
  } else {
    LAND_ERROR("Unknown Transformation Type: {}", transform_type);
    return glm::mat4{1.0f};
  }
}

glm::mat4 XmlComposeTransformMatrix(tinyxml2::XMLElement *object_element) {
  glm::mat4 result{1.0f};
  for (auto child_element = object_element->FirstChildElement("transform");
       child_element;
       child_element = child_element->NextSiblingElement("transform")) {
    result *= XmlTransformMatrix(child_element);
  }
  return result;
}
}  // namespace sparks
