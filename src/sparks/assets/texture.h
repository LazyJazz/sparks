#pragma once
#include "glm/glm.hpp"
#include "string"
#include "vector"

namespace sparks {
class Texture {
 public:
  Texture(uint32_t width = 1,
          uint32_t height = 1,
          const glm::vec3 &color = {1.0f, 1.0f, 1.0f});
  explicit Texture(uint32_t width,
                   uint32_t height,
                   const glm::vec3 *color_buffer);
  void Resize(uint32_t width, uint32_t height);
  static bool Load(const std::string &file_path, Texture &texture);
  void Store(const std::string &file_path);

 private:
  uint32_t width_{};
  uint32_t height_{};
  std::vector<glm::vec3> buffer_;
};
}  // namespace sparks
