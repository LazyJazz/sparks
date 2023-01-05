#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "glm/gtc/matrix_transform.hpp"
#include "iostream"
#include "sparks/sparks.h"
#include "tiny_obj_loader.h"

ABSL_FLAG(bool,
          validation_layer,
          grassland::vulkan::kDefaultEnableValidationLayers,
          "Enable Vulkan validation layer");
ABSL_FLAG(uint32_t, width, 1920, "Window width");
ABSL_FLAG(uint32_t, height, 1080, "Window height");
ABSL_FLAG(bool, vkrt, false, "Use Vulkan Ray Tracing pipeline");
ABSL_FLAG(int, device, -1, "Select physical device manually");

void RunApp(sparks::Renderer *renderer);

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage("Usage");
  absl::ParseCommandLine(argc, argv);
  sparks::RendererSettings renderer_settings;
  sparks::Renderer renderer(renderer_settings);
  RunApp(&renderer);
}

void RunApp(sparks::Renderer *renderer) {
#if defined(_WIN32)
  SetConsoleOutputCP(65001);
#endif
  sparks::AppSettings app_settings;
  app_settings.validation_layer = absl::GetFlag(FLAGS_validation_layer);
  app_settings.width = absl::GetFlag(FLAGS_width);
  app_settings.height = absl::GetFlag(FLAGS_height);
  app_settings.hardware_renderer = absl::GetFlag(FLAGS_vkrt);
  app_settings.selected_device = absl::GetFlag(FLAGS_device);
  sparks::App app(renderer, app_settings);
  app.Run();
}
