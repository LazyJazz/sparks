#pragma once
#include "grassland/grassland.h"
#include "object_sampler_info.h"
#include "sparks/app/app_settings.h"
#include "sparks/app/entity_device_asset.h"
#include "sparks/app/entity_uniform_object.h"
#include "sparks/app/global_uniform_object.h"
#include "sparks/app/object_info.h"
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

  void OpenFile(const std::string &file_path);
  void Capture(const std::string &file_path);
  void UpdateImGui();
  void UpdateDynamicBuffer();
  void UpdateHostStencilBuffer();
  void UpdateDeviceAssets();
  void HandleImGuiIO();
  bool UpdateImGuizmo();
  void UpdateCamera();
  void UploadAccumulationResult();
  void UpdateTopLevelAccelerationStructure();
  void UpdateObjectInfo();
  void UpdateRenderNodes();

  void RebuildRenderNodes();
  void BuildRayTracingPipeline();

  Renderer *renderer_{nullptr};
  AppSettings app_settings_{};

  std::unique_ptr<vulkan::framework::Core> core_;
  std::unique_ptr<vulkan::framework::TextureImage> screen_frame_;
  std::unique_ptr<vulkan::framework::TextureImage> render_frame_;

  std::unique_ptr<vulkan::framework::RenderNode> preview_render_node_;
  std::unique_ptr<vulkan::framework::RenderNode> preview_render_node_far_;
  std::unique_ptr<vulkan::framework::TextureImage> depth_buffer_;
  std::unique_ptr<vulkan::framework::TextureImage> stencil_buffer_;
  std::unique_ptr<vulkan::Buffer> stencil_device_buffer_;
  std::unique_ptr<vulkan::Buffer> render_frame_device_buffer_;
  std::unique_ptr<vulkan::framework::DynamicBuffer<GlobalUniformObject>>
      global_uniform_buffer_;
  std::unique_ptr<vulkan::framework::DynamicBuffer<GlobalUniformObject>>
      global_uniform_buffer_far_;
  std::unique_ptr<vulkan::framework::StaticBuffer<EntityUniformObject>>
      entity_uniform_buffer_;
  std::unique_ptr<vulkan::framework::StaticBuffer<Material>>
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

  std::vector<
      std::unique_ptr<vulkan::raytracing::BottomLevelAccelerationStructure>>
      bottom_level_acceleration_structures_;
  std::unique_ptr<vulkan::raytracing::TopLevelAccelerationStructure>
      top_level_acceleration_structure_;
  std::unique_ptr<vulkan::framework::RayTracingRenderNode>
      ray_tracing_render_node_;
  std::unique_ptr<vulkan::framework::StaticBuffer<ObjectInfo>>
      object_info_buffer_;
  std::unique_ptr<vulkan::framework::StaticBuffer<Vertex>>
      ray_tracing_vertex_buffer_;
  std::unique_ptr<vulkan::framework::StaticBuffer<uint32_t>>
      ray_tracing_index_buffer_;
  std::vector<ObjectInfo> object_info_data_;
  std::vector<Vertex> ray_tracing_vertex_data_;
  std::vector<uint32_t> ray_tracing_index_data_;

  std::vector<EntityDeviceAsset> entity_device_assets_;
  int num_loaded_device_assets_{0};

  std::vector<std::pair<std::unique_ptr<vulkan::framework::TextureImage>,
                        std::unique_ptr<vulkan::Sampler>>>
      device_texture_samplers_;
  int num_loaded_device_textures_{0};

  bool envmap_require_configure_{true};

  bool global_settings_window_open_{true};
  int hover_entity_id_{-1};
  int selected_entity_id_{-1};
  glm::vec4 hovering_pixel_color_{0.0f};
  int cursor_x_{-1};
  int cursor_y_{-1};

  bool output_render_result_{false};
  bool reset_accumulation_{true};
  bool rebuild_ray_tracing_pipeline_{false};
  uint32_t accumulated_sample_{0};
  bool gui_pause_{false};

  bool rebuild_render_nodes_{true};
  bool rebuild_object_infos_{true};
  std::unique_ptr<vulkan::framework::StaticBuffer<float>> primitive_cdf_buffer_;
  std::unique_ptr<vulkan::framework::StaticBuffer<float>> envmap_cdf_buffer_;
  float total_power_{0.0f};
  int output_selection_{0};
};
}  // namespace sparks
