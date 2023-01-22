#pragma once

namespace sparks {
struct RendererSettings {
  int num_samples{12};
  int num_bounces{10};
  float envmap_scale{1.0f};
  bool enable_mis{true};
};
}  // namespace sparks
