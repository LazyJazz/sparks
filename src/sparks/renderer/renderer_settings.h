#pragma once

namespace sparks {
struct RendererSettings {
  int num_samples{1};
  int num_bounces{16};
  bool enable_multiple_importance_sampling{true};
};
}  // namespace sparks
