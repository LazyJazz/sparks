#include "sparks/renderer/path_tracer.h"

#include "sparks/util/util.h"

namespace sparks {
PathTracer::PathTracer(const RendererSettings *render_settings,
                       const Scene *scene) {
  render_settings_ = render_settings;
  scene_ = scene;
}

glm::vec3 PathTracer::SampleRay(glm::vec3 origin,
                                glm::vec3 direction,
                                int x,
                                int y,
                                int sample) const {
  glm::vec3 throughput{1.0f};
  glm::vec3 emission{0.0f};
  HitRecord hit_record;
  const int max_bounce = render_settings_->num_bounces;
  std::mt19937 rd(sample ^ x ^ y);
  for (int i = 0; i < max_bounce; i++) {
    auto t = scene_->TraceRay(origin, direction, 1e-3f, 1e3f, &hit_record);
    if (t > 0.0f) {
      auto &material =
          scene_->GetEntity(hit_record.hit_entity_id).GetMaterial();
      throughput *=
          material.albedo_color *
          glm::vec3{scene_->GetTextures()[material.albedo_texture_id].Sample(
              hit_record.tex_coord)};
      origin = hit_record.position;
      direction = scene_->GetEnvmapLightDirection();
      emission += throughput * scene_->GetEnvmapMinorColor();
      throughput *=
          std::max(glm::dot(direction, hit_record.normal), 0.0f) * 2.0f;
      if (scene_->TraceRay(origin, direction, 1e-3f, 1e3f, nullptr) < 0.0f) {
        emission += throughput * scene_->GetEnvmapMajorColor();
      }
      break;
    } else {
      emission += throughput * glm::vec3{scene_->SampleEnvmap(direction)};
      break;
    }
  }
  return emission;
}
}  // namespace sparks
