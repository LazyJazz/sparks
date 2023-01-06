#pragma once
#include "glm/glm.hpp"
#include "string"
#include "tinyxml2.h"

namespace sparks {
glm::vec3 DecomposeRotation(glm::mat3 R);

glm::mat4 ComposeRotation(glm::vec3 pitch_yaw_roll);

glm::vec2 StringToVec2(const std::string &s);

glm::vec3 StringToVec3(const std::string &s);

glm::vec4 StringToVec4(const std::string &s);

glm::mat4 XmlTransformMatrix(tinyxml2::XMLElement *transform_element);

glm::mat4 XmlComposeTransformMatrix(tinyxml2::XMLElement *object_element);

}  // namespace sparks
