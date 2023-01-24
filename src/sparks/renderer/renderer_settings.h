#pragma once

namespace sparks {
struct RendererSettings {
  int num_samples{12};
  int num_bounces{10};
  float envmap_scale{1.0f};
  bool enable_mis{true};
  bool enable_alpha_shadow{true};
  int output_selection{0};
};
}  // namespace sparks
