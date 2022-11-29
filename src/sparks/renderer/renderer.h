#pragma once
#include "sparks/assets/assets.h"
#include "sparks/renderer/renderer_settings.h"
#include "sparks/util/util.h"

namespace sparks {
class Renderer {
 public:
  explicit Renderer(const RendererSettings &renderer_settings);
  Scene &GetScene();
  [[nodiscard]] const Scene &GetScene() const;
  RendererSettings &GetRendererSettings();
  [[nodiscard]] const RendererSettings &GetRendererSettings() const;

 private:
  RendererSettings renderer_settings_;
  Scene scene_;
};
}  // namespace sparks
