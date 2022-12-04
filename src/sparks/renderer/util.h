#pragma once
#include "cstdint"

namespace sparks {

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
  uint32_t sample;
};
}  // namespace sparks
