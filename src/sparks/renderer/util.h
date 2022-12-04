#pragma once
#include "cstdint"

namespace sparks {
constexpr float PI = 3.14159265358979323f;
constexpr float INV_PI = 0.31830988618379067f;

typedef enum RenderStateSignal : uint32_t {
  RENDER_STATE_SIGNAL_RUN = 0,
  RENDER_STATE_SIGNAL_PAUSE = 1,
  RENDER_STATE_SIGNAL_EXIT = 2
} RenderStateSignal;

struct TaskInfo {
  uint32_t x;
  uint32_t y;
  uint32_t width;
  uint32_t height;
};
}  // namespace sparks
