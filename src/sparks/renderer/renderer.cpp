#include "sparks/renderer/renderer.h"

namespace sparks {

Renderer::Renderer(const RendererSettings &renderer_settings) {
  renderer_settings_ = renderer_settings;
}

Scene &Renderer::GetScene() {
  return scene_;
}

const Scene &Renderer::GetScene() const {
  return scene_;
}

RendererSettings &Renderer::GetRendererSettings() {
  return renderer_settings_;
}

const RendererSettings &Renderer::GetRendererSettings() const {
  return renderer_settings_;
}

}  // namespace sparks
