#include "sparks/assets/texture.h"

#include "absl/strings/match.h"
#include "grassland/util/util.h"
#include "stb_image.h"
#include "stb_image_write.h"

namespace sparks {

Texture::Texture(uint32_t width, uint32_t height, const glm::vec3 &color) {
  width_ = width;
  height_ = height;
  buffer_.resize(width * height);
  std::fill(buffer_.data(), buffer_.data() + width * height, color);
}

Texture::Texture(uint32_t width,
                 uint32_t height,
                 const glm::vec3 *color_buffer) {
  width_ = width;
  height_ = height;
  buffer_.resize(width * height);
  std::memcpy(buffer_.data(), color_buffer,
              sizeof(glm::vec3) * width_ * height_);
}

void Texture::Resize(uint32_t width, uint32_t height) {
  std::vector<glm::vec3> new_buffer(width * height);
  for (int i = 0; i < std::min(height, height_); i++) {
    std::memcpy(new_buffer.data() + width * i, buffer_.data() + width_ * i,
                sizeof(glm::vec3) * std::min(width, width_));
  }
  width_ = width;
  height_ = height;
  buffer_ = new_buffer;
}

bool Texture::Load(const std::string &file_path, Texture &texture) {
  int x, y, c;
  if (absl::EndsWithIgnoreCase(file_path, ".hdr")) {
    auto result = stbi_loadf(file_path.c_str(), &x, &y, &c, 3);
    if (result) {
      texture = Texture(x, y, reinterpret_cast<glm::vec3 *>(result));
      stbi_image_free(result);
    } else {
      return false;
    }
  } else {
    auto result = stbi_load(file_path.c_str(), &x, &y, &c, 3);
    if (result) {
      std::vector<glm::vec3> convert_buffer(x * y);
      const float inv_255 = 1.0f / 255.0f;
      for (int i = 0; i < x * y; i++) {
        convert_buffer[i] =
            glm::vec3{result[i * 3], result[i * 3 + 1], result[i * 3 + 2]} *
            inv_255;
      }
      texture = Texture(x, y, convert_buffer.data());
      stbi_image_free(result);
    } else {
      return false;
    }
  }
  return true;
}

void Texture::Store(const std::string &file_path) {
  if (absl::EndsWithIgnoreCase(file_path, ".hdr")) {
    stbi_write_hdr(file_path.c_str(), width_, height_, 3,
                   reinterpret_cast<float *>(buffer_.data()));
  } else {
    std::vector<uint8_t> convert_buffer(width_ * height_ * 3);
    auto float_to_uint8 = [](float x) {
      return std::min(std::max(std::lround(x * 255.0f), 0l), 255l);
    };
    for (int i = 0; i < width_ * height_; i++) {
      convert_buffer[i * 3] = float_to_uint8(buffer_[i].x);
      convert_buffer[i * 3 + 1] = float_to_uint8(buffer_[i].y);
      convert_buffer[i * 3 + 2] = float_to_uint8(buffer_[i].z);
    }
    if (absl::EndsWithIgnoreCase(file_path, ".png")) {
      stbi_write_png(file_path.c_str(), width_, height_, 3,
                     convert_buffer.data(), width_ * 3);
    } else if (absl::EndsWithIgnoreCase(file_path, ".bmp")) {
      stbi_write_bmp(file_path.c_str(), width_, height_, 3,
                     convert_buffer.data());
    } else if (absl::EndsWithIgnoreCase(file_path, ".jpg") ||
               absl::EndsWithIgnoreCase(file_path, ".jpeg")) {
      stbi_write_jpg(file_path.c_str(), width_, height_, 3,
                     convert_buffer.data(), 100);
    } else {
      LAND_ERROR("Unknown file format \"{}\"", file_path.c_str());
    }
  }
}
}  // namespace sparks
