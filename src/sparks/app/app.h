#pragma once
#include "grassland/grassland.h"
#include "sparks/app/app_settings.h"
#include "sparks/app/entity_device_asset.h"
#include "sparks/app/entity_uniform_object.h"
#include "sparks/app/global_uniform_object.h"
#include "sparks/renderer/renderer.h"

namespace sparks {
using namespace grassland;
class App {
 public:
  explicit App(Renderer *renderer, const AppSettings &app_settings);
  void Run();

 private:
  void OnInit();
  void OnLoop();
  void OnUpdate(uint32_t ms);
  void OnRender();
  void OnClose();

  void UpdateImGui();
  void UpdateDynamicBuffer();
  void UpdateHostStencilBuffer();
  void UpdateDeviceAssets();
  void HandleImGuiIO();
  void UpdateImGuizmo();
  void UpdateCamera();
  void UpdateEnvmapConfiguration();
  void UploadAccumulationResult();

  void RebuildRenderNode();

  Renderer *renderer_{nullptr};

  std::unique_ptr<vulkan::framework::Core> core_;
  std::unique_ptr<vulkan::framework::TextureImage> screen_frame_;
  std::unique_ptr<vulkan::framework::TextureImage> render_frame_;

  std::unique_ptr<vulkan::framework::RenderNode> render_node_;
  std::unique_ptr<vulkan::framework::TextureImage> depth_buffer_;
  std::unique_ptr<vulkan::framework::TextureImage> stencil_buffer_;
  std::unique_ptr<vulkan::Buffer> stencil_device_buffer_;
  std::vector<uint32_t> stencil_host_buffer_;
  std::unique_ptr<vulkan::framework::DynamicBuffer<GlobalUniformObject>>
      global_uniform_buffer_;
  std::unique_ptr<vulkan::framework::DynamicBuffer<EntityUniformObject>>
      entity_uniform_buffer_;
  std::unique_ptr<vulkan::framework::DynamicBuffer<Material>>
      material_uniform_buffer_;

  std::unique_ptr<vulkan::framework::RenderNode> host_result_render_node_;
  std::unique_ptr<vulkan::framework::TextureImage> accumulation_color_;
  std::unique_ptr<vulkan::framework::TextureImage> accumulation_number_;
  std::unique_ptr<vulkan::Buffer> host_accumulation_color_;
  std::unique_ptr<vulkan::Buffer> host_accumulation_number_;

  std::unique_ptr<vulkan::framework::RenderNode> envmap_render_node_;
  std::unique_ptr<vulkan::framework::RenderNode> postproc_render_node_;
  std::unique_ptr<vulkan::framework::StaticBuffer<glm::vec2>>
      envmap_vertex_buffer_;
  std::unique_ptr<vulkan::framework::StaticBuffer<uint32_t>>
      envmap_index_buffer_;

  std::unique_ptr<vulkan::Sampler> linear_sampler_;
  std::unique_ptr<vulkan::Sampler> nearest_sampler_;

  std::vector<EntityDeviceAsset> entity_device_assets_;
  int num_loaded_device_assets_{0};

  std::vector<std::pair<std::unique_ptr<vulkan::framework::TextureImage>,
                        std::unique_ptr<vulkan::Sampler>>>
      device_texture_samplers_;
  int num_loaded_device_textures_{0};

  std::vector<float> envmap_cdf_;
  glm::vec3 envmap_light_direction_{0.0f, 1.0f, 0.0f};
  glm::vec3 envmap_major_color_{0.5f};
  glm::vec3 envmap_minor_color_{0.3f};
  bool envmap_require_configure_{true};

  bool global_settings_window_open_{true};
  int hover_entity_id_{-1};
  int selected_entity_id_{-1};

  bool output_render_result_{false};
};
}  // namespace sparks
