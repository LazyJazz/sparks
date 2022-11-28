#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "iostream"
#include "sparks/sparks.h"

ABSL_FLAG(bool,
          validation_layer,
          grassland::vulkan::kDefaultEnableValidationLayers,
          "Enable Vulkan validation layer");
ABSL_FLAG(uint32_t, width, 1920, "Window width");
ABSL_FLAG(uint32_t, height, 1080, "Window height");

void RunApp();

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage("Usage");
  absl::ParseCommandLine(argc, argv);

  RunApp();
}

void RunApp() {
#if defined(_WIN32)
  SetConsoleOutputCP(65001);
#endif
  sparks::AppSettings app_settings;
  app_settings.validation_layer = absl::GetFlag(FLAGS_validation_layer);
  app_settings.width = absl::GetFlag(FLAGS_width);
  app_settings.height = absl::GetFlag(FLAGS_height);
  sparks::App app(app_settings);
  app.Run();
}
