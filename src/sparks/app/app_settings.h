#pragma once

namespace sparks {
struct AppSettings {
  uint32_t width{};
  uint32_t height{};
  bool validation_layer{};
  bool hardware_renderer{};
  int selected_device{-1};
};
}  // namespace sparks
