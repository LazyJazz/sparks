#pragma once
#include "grassland/grassland.h"

namespace sparks {
using namespace grassland;
class App {
 public:
  App();
  void Run();

 private:
  void OnInit();
  void OnLoop();
  void OnUpdate(uint32_t ms);
  void OnRender();
  void OnClose();

  void UpdateImGui();

  std::unique_ptr<vulkan::framework::Core> core_;
  std::unique_ptr<vulkan::framework::TextureImage> screen_frame_;
};
}  // namespace sparks
