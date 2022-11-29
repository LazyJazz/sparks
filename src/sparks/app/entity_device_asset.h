#pragma once
#include "grassland/grassland.h"
#include "sparks/assets/vertex.h"

namespace sparks {
using namespace grassland;
struct EntityDeviceAsset {
  std::unique_ptr<vulkan::framework::StaticBuffer<Vertex>> vertex_buffer;
  std::unique_ptr<vulkan::framework::StaticBuffer<uint32_t>> index_buffer;
};
}  // namespace sparks
