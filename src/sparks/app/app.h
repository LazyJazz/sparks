#pragma once
#include "grassland/grassland.h"
#include "sparks/app/app_settings.h"
#include "sparks/renderer/renderer.h"

namespace sparks {
using namespace grassland;
class App {
 public:
  explicit App(Renderer *renderer, const AppSettings &app_settings);
  void Run();

 private:
  void OnInit();
  void OnLoop();
  void OnUpdate(uint32_t ms);
  void OnRender();
  void OnClose();

  void UpdateImGui();

  Renderer *renderer_{nullptr};

  std::unique_ptr<vulkan::framework::Core> core_;
  std::unique_ptr<vulkan::framework::TextureImage> screen_frame_;

  bool global_settings_window_open_{true};
};
}  // namespace sparks
