#pragma once

namespace sparks {
struct RendererSettings {
  int num_samples{1};
  int num_bounces{16};
  bool enable_mis{true};
};
}  // namespace sparks
