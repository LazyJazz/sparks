#include "sparks/app/app.h"

#include "ImGuizmo.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "cmath"
#include "glm/gtc/matrix_transform.hpp"
#include "iostream"
#include "sparks/util/util.h"
#include "stb_image_write.h"
#include "tinyfiledialogs.h"

namespace sparks {

App::App(Renderer *renderer, const AppSettings &app_settings) {
  renderer_ = renderer;
  app_settings_ = app_settings;
  vulkan::framework::CoreSettings core_settings;
  core_settings.window_title = "Sparks";
  core_settings.window_width = app_settings_.width;
  core_settings.window_height = app_settings_.height;
  core_settings.validation_layer = app_settings_.validation_layer;
  core_settings.raytracing_pipeline_required = app_settings_.hardware_renderer;
  core_settings.selected_device = app_settings.selected_device;
  core_ = std::make_unique<vulkan::framework::Core>(core_settings);
}

void App::Run() {
  OnInit();
  while (!glfwWindowShouldClose(core_->GetWindow())) {
    if (!gui_pause_) {
      OnLoop();
    }
    glfwPollEvents();
  }
  core_->GetDevice()->WaitIdle();
  OnClose();
}

void App::OnInit() {
  if (app_settings_.hardware_renderer) {
  } else {
    LAND_INFO("Starting worker threads.");
    renderer_->StartWorkerThreads();
  }

  LAND_INFO("Allocating visual pipeline textures.");
  screen_frame_ = std::make_unique<vulkan::framework::TextureImage>(
      core_.get(), core_->GetFramebufferWidth(), core_->GetFramebufferHeight(),
      VK_FORMAT_B8G8R8A8_UNORM,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  render_frame_ = std::make_unique<vulkan::framework::TextureImage>(
      core_.get(), core_->GetFramebufferWidth(), core_->GetFramebufferHeight(),
      VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  depth_buffer_ = std::make_unique<vulkan::framework::TextureImage>(
      core_.get(), core_->GetFramebufferWidth(), core_->GetFramebufferHeight(),
      VK_FORMAT_D32_SFLOAT,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  stencil_buffer_ = std::make_unique<vulkan::framework::TextureImage>(
      core_.get(), core_->GetFramebufferWidth(), core_->GetFramebufferHeight(),
      VK_FORMAT_R32_UINT,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
  stencil_device_buffer_ = std::make_unique<vulkan::Buffer>(
      core_->GetDevice(),
      sizeof(uint32_t) * core_->GetFramebufferWidth() *
          core_->GetFramebufferHeight(),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  render_frame_device_buffer_ = std::make_unique<vulkan::Buffer>(
      core_->GetDevice(),
      sizeof(glm::vec4) * core_->GetFramebufferWidth() *
          core_->GetFramebufferHeight(),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  renderer_->Resize(core_->GetFramebufferWidth(),
                    core_->GetFramebufferHeight());
  accumulation_number_ = std::make_unique<vulkan::framework::TextureImage>(
      core_.get(), core_->GetFramebufferWidth(), core_->GetFramebufferHeight(),
      VK_FORMAT_R32_SFLOAT,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_STORAGE_BIT);
  accumulation_color_ = std::make_unique<vulkan::framework::TextureImage>(
      core_.get(), core_->GetFramebufferWidth(), core_->GetFramebufferHeight(),
      VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_STORAGE_BIT);
  host_accumulation_color_ = std::make_unique<vulkan::Buffer>(
      core_->GetDevice(),
      core_->GetFramebufferWidth() * core_->GetFramebufferHeight() *
          sizeof(glm::vec4),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  host_accumulation_number_ = std::make_unique<vulkan::Buffer>(
      core_->GetDevice(),
      core_->GetFramebufferWidth() * core_->GetFramebufferHeight() *
          sizeof(float),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  core_->SetFrameSizeCallback([this](int width, int height) {
    core_->GetDevice()->WaitIdle();
    gui_pause_ = !(width && height);
    if (gui_pause_) {
      return;
    }
    screen_frame_->Resize(width, height);
    render_frame_->Resize(width, height);
    depth_buffer_->Resize(width, height);
    stencil_buffer_->Resize(width, height);
    preview_render_node_->BuildRenderNode(width, height);
    preview_render_node_far_->BuildRenderNode(width, height);
    envmap_render_node_->BuildRenderNode(width, height);
    postproc_render_node_->BuildRenderNode(width, height);
    stencil_device_buffer_ = std::make_unique<vulkan::Buffer>(
        core_->GetDevice(), sizeof(uint32_t) * width * height,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    render_frame_device_buffer_ = std::make_unique<vulkan::Buffer>(
        core_->GetDevice(), sizeof(glm::vec4) * width * height,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    renderer_->Resize(width, height);
    accumulation_color_->Resize(width, height);
    accumulation_number_->Resize(width, height);
    host_accumulation_color_ = std::make_unique<vulkan::Buffer>(
        core_->GetDevice(), width * height * sizeof(glm::vec4),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    host_accumulation_number_ = std::make_unique<vulkan::Buffer>(
        core_->GetDevice(), width * height * sizeof(float),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    host_result_render_node_->BuildRenderNode(width, height);
    if (app_settings_.hardware_renderer) {
      ray_tracing_render_node_->BuildRenderNode();
    }
    reset_accumulation_ = true;
  });

  core_->SetDropCallback([this](int path_count, const char **paths) {
    for (int i = 0; i < path_count; i++) {
      auto path = paths[i];
      LAND_INFO("Loading asset: {}", path);
      OpenFile(path);
    }
  });

  LAND_INFO("Initializing ImGui.");
  core_->ImGuiInit(screen_frame_.get(), "../../fonts/NotoSansSC-Regular.otf",
                   24.0f);

  LAND_INFO("Allocating visual pipeline buffers.");
  global_uniform_buffer_ =
      std::make_unique<vulkan::framework::DynamicBuffer<GlobalUniformObject>>(
          core_.get(), 1);
  global_uniform_buffer_far_ =
      std::make_unique<vulkan::framework::DynamicBuffer<GlobalUniformObject>>(
          core_.get(), 1);
  entity_uniform_buffer_ =
      std::make_unique<vulkan::framework::StaticBuffer<EntityUniformObject>>(
          core_.get(), 1);
  material_uniform_buffer_ =
      std::make_unique<vulkan::framework::StaticBuffer<Material>>(core_.get(),
                                                                  1);

  std::vector<glm::vec2> vertices{
      {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}};
  std::vector<uint32_t> indices{0, 1, 2, 1, 2, 3};
  envmap_vertex_buffer_ =
      std::make_unique<vulkan::framework::StaticBuffer<glm::vec2>>(core_.get(),
                                                                   4);
  envmap_index_buffer_ =
      std::make_unique<vulkan::framework::StaticBuffer<uint32_t>>(core_.get(),
                                                                  6);
  envmap_vertex_buffer_->Upload(vertices.data());
  envmap_index_buffer_->Upload(indices.data());

  linear_sampler_ = std::make_unique<vulkan::Sampler>(
      core_->GetDevice(), VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
  nearest_sampler_ = std::make_unique<vulkan::Sampler>(
      core_->GetDevice(), VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);

  primitive_cdf_buffer_ =
      std::make_unique<vulkan::framework::StaticBuffer<float>>(core_.get(), 1);
  envmap_cdf_buffer_ =
      std::make_unique<vulkan::framework::StaticBuffer<float>>(core_.get(), 1);

  if (app_settings_.hardware_renderer) {
    auto sobol_table =
        SobolTableGen(65536, 1024, "../../sobol/new-joe-kuo-7.21201");
    sobol_table_buffer_ =
        std::make_unique<vulkan::framework::StaticBuffer<uint32_t>>(
            core_.get(), sobol_table.size());
    sobol_table_buffer_->Upload(sobol_table.data());
  }

  ImGuizmo::Enable(true);
}

void App::OnLoop() {
  static auto last_tp = std::chrono::steady_clock::now();
  auto cur_tp = std::chrono::steady_clock::now();
  auto dur = cur_tp - last_tp;
  auto ms = dur / std::chrono::milliseconds(1);
  last_tp += std::chrono::milliseconds(ms);
  OnUpdate(ms);
  OnRender();
}

void App::OnUpdate(uint32_t ms) {
  UpdateImGui();
  if (envmap_require_configure_) {
    renderer_->SafeOperation<void>([&]() {
      renderer_->GetScene().UpdateEnvmapConfiguration();
      envmap_require_configure_ = false;
    });
    if (app_settings_.hardware_renderer) {
      envmap_cdf_buffer_->Resize(renderer_->GetScene().GetEnvmapCdf().size());
      envmap_cdf_buffer_->Upload(renderer_->GetScene().GetEnvmapCdf().data());
      if (ray_tracing_render_node_) {
        ray_tracing_render_node_->UpdateDescriptorSetBinding(11);
      }
    }
  }
  UpdateDynamicBuffer();
  UpdateHostStencilBuffer();
  UpdateDeviceAssets();
  HandleImGuiIO();
  UpdateCamera();
  if (output_render_result_) {
    UploadAccumulationResult();
  }
  if (reset_accumulation_) {
    core_->GetDevice()->WaitIdle();
    if (!app_settings_.hardware_renderer) {
      renderer_->ResetAccumulation();
      reset_accumulation_ = false;
    }
  }
  if (app_settings_.hardware_renderer) {
    UpdateTopLevelAccelerationStructure();
  }
  if (rebuild_ray_tracing_pipeline_) {
    BuildRayTracingPipeline();
    rebuild_ray_tracing_pipeline_ = false;
  }
  UpdateRenderNodes();
}

void App::OnRender() {
  core_->BeginCommandRecord();
  render_frame_->ClearColor({0.0f, 0.0f, 0.0f, 1.0f});
  VkClearColorValue stencil_clear_color{};
  stencil_clear_color.uint32[0] = 0xffffffffu;
  stencil_buffer_->ClearColor(stencil_clear_color);
  depth_buffer_->ClearDepth({1.0f, 0});
  envmap_render_node_->Draw(envmap_vertex_buffer_.get(),
                            envmap_index_buffer_.get(),
                            envmap_index_buffer_->Size(), 0);
  preview_render_node_far_->BeginDraw();
  for (int i = 0; i < entity_device_assets_.size(); i++) {
    auto &entity_asset = entity_device_assets_[i];
    preview_render_node_far_->DrawDirect(entity_asset.vertex_buffer.get(),
                                         entity_asset.index_buffer.get(),
                                         entity_asset.index_buffer->Size(), i);
  }
  preview_render_node_far_->EndDraw();
  depth_buffer_->ClearDepth({1.0f, 0});
  preview_render_node_->BeginDraw();
  for (int i = 0; i < entity_device_assets_.size(); i++) {
    auto &entity_asset = entity_device_assets_[i];
    preview_render_node_->DrawDirect(entity_asset.vertex_buffer.get(),
                                     entity_asset.index_buffer.get(),
                                     entity_asset.index_buffer->Size(), i);
  }
  preview_render_node_->EndDraw();
  if (app_settings_.hardware_renderer) {
    ray_tracing_render_node_->Draw();
    accumulated_sample_ += renderer_->GetRendererSettings().num_samples;
  }
  if (output_render_result_) {
    host_result_render_node_->Draw(envmap_vertex_buffer_.get(),
                                   envmap_index_buffer_.get(),
                                   envmap_index_buffer_->Size(), 0);
  }
  postproc_render_node_->Draw(envmap_vertex_buffer_.get(),
                              envmap_index_buffer_.get(),
                              envmap_index_buffer_->Size(), 0);
  auto command_buffer = core_->GetCommandBuffer();
  vulkan::TransitImageLayout(
      command_buffer->GetHandle(), stencil_buffer_->GetImage()->GetHandle(),
      VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_TRANSFER_READ_BIT);
  stencil_buffer_->GetImage()->Retrieve(command_buffer,
                                        stencil_device_buffer_.get());
  vulkan::TransitImageLayout(
      command_buffer->GetHandle(), stencil_buffer_->GetImage()->GetHandle(),
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

  vulkan::TransitImageLayout(
      command_buffer->GetHandle(), render_frame_->GetImage()->GetHandle(),
      VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_TRANSFER_READ_BIT);
  render_frame_->GetImage()->Retrieve(command_buffer,
                                      render_frame_device_buffer_.get());
  vulkan::TransitImageLayout(
      command_buffer->GetHandle(), render_frame_->GetImage()->GetHandle(),
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
  core_->TemporalSubmit();
  core_->ImGuiRender();
  core_->Output(screen_frame_.get());

  if (app_settings_.hardware_renderer && reset_accumulation_) {
    accumulation_number_->ClearColor({0.0f, 0.0f, 0.0f, 0.0f});
    accumulation_color_->ClearColor({0.0f, 0.0f, 0.0f, 0.0f});
    reset_accumulation_ = false;
    accumulated_sample_ = 0;
  }
  core_->EndCommandRecordAndSubmit();
}

void App::OnClose() {
  entity_device_assets_.clear();
  preview_render_node_.reset();
  stencil_buffer_.reset();
  global_uniform_buffer_.reset();
  global_uniform_buffer_far_.reset();
  entity_uniform_buffer_.reset();
  screen_frame_.reset();
  if (app_settings_.hardware_renderer) {
  } else {
    renderer_->StopWorkers();
  }
}

void App::UpdateImGui() {
  uint32_t current_sample;
  if (app_settings_.hardware_renderer) {
    current_sample = accumulated_sample_;
  } else {
    current_sample = renderer_->GetAccumulatedSamples();
  }

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGuizmo::BeginFrame();

  auto &io = ImGui::GetIO();
  auto &scene = renderer_->GetScene();
  if (ImGui::IsKeyPressed(ImGuiKey_G)) {
    global_settings_window_open_ = !global_settings_window_open_;
  }

  if (global_settings_window_open_) {
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Once);
    ImGui::SetNextWindowBgAlpha(0.3);
    ImGui::Begin("Global Settings", &global_settings_window_open_,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        char *result{nullptr};
        if (ImGui::MenuItem("Open Scene")) {
          std::vector<const char *> file_types = {"*.xml"};
          result = tinyfd_openFileDialog("Open Scene", "", file_types.size(),
                                         file_types.data(),
                                         "Scene files (*.xml)", 0);
        }
        if (ImGui::MenuItem("Import Image..")) {
          std::vector<const char *> file_types = {"*.bmp", "*.png", "*.jpg",
                                                  "*.hdr"};
          result = tinyfd_openFileDialog(
              "Import Image", "", file_types.size(), file_types.data(),
              "Image Files (*.bmp, *.jpg, *.png, *hdr)", 1);
        }
        if (ImGui::MenuItem("Import Mesh..")) {
          std::vector<const char *> file_types = {"*.obj"};
          result =
              tinyfd_openFileDialog("Import Mesh", "", file_types.size(),
                                    file_types.data(), "Mesh Files (*.obj)", 1);
        }
        if (result) {
          std::vector<std::string> file_paths = absl::StrSplit(result, "|");
          for (auto &file_path : file_paths) {
            OpenFile(file_path);
          }
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Capture and Save..")) {
          std::vector<const char *> file_types = {"*.bmp", "*.png", "*.jpg",
                                                  "*.hdr"};
          result = tinyfd_saveFileDialog(
              "Save Captured Image", "", file_types.size(), file_types.data(),
              "Image Files (*.bmp, *.jpg, *.png, *hdr)");
          if (result) {
            Capture(result);
          }
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }

    ImGui::Text("Notice:");
    ImGui::Text("W/A/S/D/LCTRL/SPACE for camera movement.");
    ImGui::Text("Cursor drag on frame for camera rotation.");
    ImGui::Text("G for show/hide GUI windows.");

    ImGui::NewLine();
    ImGui::Text("Renderer");
    ImGui::Separator();
    if (ImGui::RadioButton("Preview", !output_render_result_)) {
      output_render_result_ = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Renderer", output_render_result_)) {
      output_render_result_ = true;
    }

    if (app_settings_.hardware_renderer) {
    } else {
      ImGui::SameLine();
      if (renderer_->IsPaused()) {
        if (ImGui::Button("Resume")) {
          renderer_->ResumeWorkers();
        }
      } else {
        if (ImGui::Button("Pause")) {
          renderer_->PauseWorkers();
        }
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Quick Capture")) {
      auto tt = std::chrono::system_clock::to_time_t(
          std::chrono::system_clock::now());
#ifdef _WIN32
      auto now = std::make_unique<std::tm>();
      localtime_s(now.get(), &tt);
#else
      std::tm *now = std::localtime(&tt);
#endif
      std::string time_string;
      Capture("screenshot_" + std::to_string(now->tm_year + 1900) + "_" +
              std::to_string(now->tm_mon) + "_" + std::to_string(now->tm_mday) +
              "_" + std::to_string(now->tm_hour) + "_" +
              std::to_string(now->tm_min) + "_" + std::to_string(now->tm_sec) +
              "_" + std::to_string(current_sample) + "spp.png");
    }

    auto &renderer_settings = renderer_->GetRendererSettings();
    if (app_settings_.hardware_renderer) {
      reset_accumulation_ |=
          ImGui::SliderInt("Samples", &renderer_settings.num_samples, 1, 128);
    } else {
      reset_accumulation_ |=
          ImGui::SliderInt("Samples", &renderer_settings.num_samples, 1, 16);
    }
    reset_accumulation_ |=
        ImGui::SliderInt("Bounces", &renderer_settings.num_bounces, 1, 128);
    reset_accumulation_ |=
        ImGui::Checkbox("Enable MIS", &renderer_settings.enable_mis);
    reset_accumulation_ |=
        ImGui::Checkbox("Alpha Shadow", &renderer_settings.enable_alpha_shadow);

    scene.EntityCombo("Selected Entity", &selected_entity_id_);

    std::vector<const char *> output_type = {
        "Color",          "Normal",          "Tangent",     "Bitangent",
        "Shading Normal", "Geometry Normal", "Front Facing"};
    reset_accumulation_ |= ImGui::Combo(
        "Output Channel",
        reinterpret_cast<int *>(&renderer_settings.output_selection),
        output_type.data(), output_type.size());

    ImGui::NewLine();
    ImGui::Text("Camera");
    ImGui::Separator();
    reset_accumulation_ |= scene.GetCamera().ImGuiItems();
    reset_accumulation_ |= ImGui::InputFloat3(
        "Position", reinterpret_cast<float *>(&scene.GetCameraPosition()));
    ImGui::SliderFloat("Moving Speed", &scene.GetCameraSpeed(), 0.01f, 1e6f,
                       "%.3f", ImGuiSliderFlags_Logarithmic);
    reset_accumulation_ |= ImGui::SliderAngle(
        "Pitch", &scene.GetCameraPitchYawRoll().x, -90.0f, 90.0f);
    reset_accumulation_ |= ImGui::SliderAngle(
        "Yaw", &scene.GetCameraPitchYawRoll().y, 0.0f, 360.0f);
    reset_accumulation_ |= ImGui::SliderAngle(
        "Roll", &scene.GetCameraPitchYawRoll().z, -180.0f, 180.0f);

    ImGui::NewLine();
    ImGui::Text("Environment Map");
    ImGui::Separator();
    envmap_require_configure_ |=
        scene.TextureCombo("Envmap Texture", &scene.GetEnvmapId());
    reset_accumulation_ |= envmap_require_configure_;
    reset_accumulation_ |= ImGui::SliderAngle(
        "Offset", &scene.GetEnvmapOffset(), 0.0f, 360.0f, "%.0f deg");
    reset_accumulation_ |= ImGui::SliderFloat(
        "Envmap Scale", &renderer_->GetRendererSettings().envmap_scale, 0.0f,
        10.0f);

    if (selected_entity_id_ != -1) {
      ImGui::NewLine();
      ImGui::Text("Material");
      ImGui::Separator();
      static int current_item = 0;
      std::vector<const char *> material_types = {
          "Lambertian", "Specular", "Transmissive", "Principled", "Emission"};
      Material &material = scene.GetEntity(selected_entity_id_).GetMaterial();
      rebuild_object_infos_ |=
          ImGui::Combo("Type", reinterpret_cast<int *>(&material.material_type),
                       material_types.data(), material_types.size());
      rebuild_object_infos_ |= ImGui::ColorEdit3(
          "Albedo Color", &material.base_color[0],
          ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_Float);
      rebuild_object_infos_ |=
          scene.TextureCombo("Albedo Texture", &material.base_color_texture_id);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("Subsurface", &material.subsurface, 0.0f, 1.0f);
      rebuild_object_infos_ |= ImGui::ColorEdit3(
          "Subsurface Color", &material.subsurface_color[0],
          ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_Float);
      rebuild_object_infos_ |= ImGui::SliderFloat3(
          "Subsurface Radius", &material.subsurface_radius[0], 0.0f, 1e4f,
          "%.3f", ImGuiSliderFlags_Logarithmic);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("Specular", &material.specular, 0.0f, 1.0f);
      rebuild_object_infos_ |= ImGui::SliderFloat(
          "Specular Tint", &material.specular_tint, 0.0f, 1.0f);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("Anisotropic", &material.anisotropic, 0.0f, 1.0f);
      rebuild_object_infos_ |= ImGui::SliderFloat(
          "Anisotropic Rotation", &material.anisotropic_rotation, 0.0f, 1.0f);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("Sheen", &material.sheen, 0.0f, 1.0f);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("Sheen Tint", &material.sheen_tint, 0.0f, 1.0f);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("Clearcoat", &material.clearcoat, 0.0f, 1.0f);
      rebuild_object_infos_ |= ImGui::SliderFloat(
          "Clearcoat Roughness", &material.clearcoat_roughness, 0.0f, 1.0f);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("IOR", &material.ior, 0.0f, 10.0f);
      rebuild_object_infos_ |= ImGui::SliderFloat(
          "Transmission", &material.transmission, 0.0f, 1.0f);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("Transmission Roughness",
                             &material.transmission_roughness, 0.0f, 1.0f);
      rebuild_object_infos_ |= ImGui::ColorEdit3(
          "Emission", &material.emission[0],
          ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_Float);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("Emission Strength", &material.emission_strength,
                             0.0f, 1e5f, "%.3f", ImGuiSliderFlags_Logarithmic);
      rebuild_object_infos_ |=
          ImGui::SliderFloat("Alpha", &material.alpha, 0.0f, 1.0f, "%.3f");
      rebuild_object_infos_ |=
          scene.TextureCombo("Normal Map", &material.normal_map_id);

      if (material.normal_map_id != -1) {
        rebuild_object_infos_ |= ImGui::SliderFloat(
            "Normal Intensity", &material.normal_map_intensity, 0.0f, 10.0f);
        bool reverse_bitangent = false;
        if (material.normal_map_id < 0) {
          reverse_bitangent = true;
        }
        rebuild_object_infos_ |=
            ImGui::Checkbox("Reverse Bitangent", &reverse_bitangent);
        if (reverse_bitangent) {
          material.normal_map_id |= 0x80000000;
        } else {
          material.normal_map_id &= 0x7fffffff;
        }

        if (ImGui::Button("Remove Normal Map")) {
          material.normal_map_id = -1;
          rebuild_object_infos_ = true;
        }
      }
      if (scene.TextureCombo("Metallic Texture",
                             &material.metallic_texture_id)) {
        if (material.metallic_texture_id) {
          material.metallic = 1.0f;
        } else {
          material.metallic = 0.0f;
        }
        rebuild_object_infos_ |= true;
      }
      if (scene.TextureCombo("Roughness Texture",
                             &material.roughness_texture_id)) {
        if (material.metallic_texture_id) {
          material.roughness = 1.0f;
        } else {
          material.roughness = 0.0f;
        }
        rebuild_object_infos_ |= true;
      }
    }

#if !defined(NDEBUG)
    ImGui::NewLine();
    ImGui::Text("Debug Info");
    ImGui::Separator();
    ImGui::Text("Hover item: %s",
                (hover_entity_id_ == -1)
                    ? "None"
                    : std::to_string(hover_entity_id_).c_str());
    ImGui::Text("Selected item: %s",
                (selected_entity_id_ == -1)
                    ? "None"
                    : std::to_string(selected_entity_id_).c_str());
    ImGui::Text("Want Capture Mouse: %s", io.WantCaptureMouse ? "yes" : "no");
    ImGui::Text("Has Mouse Clicked: %s", io.MouseClicked[0] ? "yes" : "no");
    ImGui::Text("Is Any Item Active: %s",
                ImGui::IsAnyItemActive() ? "yes" : "no");
    ImGui::Text("Is Any Item Focused: %s",
                ImGui::IsAnyItemFocused() ? "yes" : "no");
    ImGui::Text("Is Any Item Hovered: %s",
                ImGui::IsAnyItemHovered() ? "yes" : "no");
    ImGui::Text("Mouse Wheel: %f", io.MouseWheel);
    ImGui::Text("Mouse Wheel H: %f", io.MouseWheelH);
    ImGui::Text("ImGuizmo Is Using: %s", ImGuizmo::IsUsing() ? "yes" : "no");
#endif

    ImGui::End();

    ImVec2 statistic_window_size{0.0f, 0.0f};
    if (selected_entity_id_ != -1) {
      ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x, 0.0),
                              ImGuiCond_Always, ImVec2(1.0f, 0.0f));
      ImGui::SetNextWindowBgAlpha(0.3);
      ImGui::Begin("Gizmo", &global_settings_window_open_,
                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoTitleBar);
      statistic_window_size = ImGui::GetWindowSize();
      rebuild_object_infos_ |= UpdateImGuizmo();
      ImGui::End();
    }

    ImGui::SetNextWindowPos(
        ImVec2(ImGui::GetIO().DisplaySize.x, statistic_window_size.y),
        ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.3f);
    ImGui::Begin("Statistics", &global_settings_window_open_,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("Statistics (%dx%d)", core_->GetFramebufferWidth(),
                core_->GetFramebufferHeight());
    ImGui::Separator();
    auto current_time = std::chrono::steady_clock::now();
    static auto last_sample = current_sample;
    static auto last_sample_time = current_time;
    static float sample_rate = 0.0f;
    float duration_us = 0;
    if (last_sample != current_sample) {
      if (last_sample < current_sample) {
        auto duration_ms =
            (current_time - last_sample_time) / std::chrono::milliseconds(1);
        duration_us = float((current_time - last_sample_time) /
                            std::chrono::microseconds(1));
        sample_rate = (float(core_->GetFramebufferWidth()) *
                       float(core_->GetFramebufferHeight()) *
                       float(renderer_->GetRendererSettings().num_samples)) /
                      (0.001f * float(duration_ms));
      } else {
        sample_rate = NAN;
      }
      last_sample = current_sample;
      last_sample_time = current_time;
    }
    if (std::isnan(sample_rate)) {
      ImGui::Text("Primary Ray Rate: N/A");
    } else if (sample_rate >= 1e9f) {
      ImGui::Text("Primary Ray Rate: %.2f Gr/s", sample_rate * 1e-9f);
    } else if (sample_rate >= 1e6f) {
      ImGui::Text("Primary Ray Rate: %.2f Mr/s", sample_rate * 1e-6f);
    } else if (sample_rate >= 1e3f) {
      ImGui::Text("Primary Ray Rate: %.2f Kr/s", sample_rate * 1e-3f);
    } else {
      ImGui::Text("Primary Ray Rate: %.2f r/s", sample_rate);
    }
    ImGui::Text("Accumulated Samples: %d", current_sample);
    ImGui::Text("Cursor Position: (%d, %d)", cursor_x_, cursor_y_);
    ImGui::Text("R:%f G:%f B:%f", hovering_pixel_color_.x,
                hovering_pixel_color_.y, hovering_pixel_color_.z);
    ImGui::Text("HEX(%08x, %08x, %08x)",
                glm::floatBitsToInt(hovering_pixel_color_.x),
                glm::floatBitsToInt(hovering_pixel_color_.y),
                glm::floatBitsToInt(hovering_pixel_color_.z));
    if (app_settings_.hardware_renderer) {
      ImGui::Text("Frame Duration: %.3lf ms", duration_us * 0.001f);
      ImGui::Text("Fps: %.2lf", 1.0f / (duration_us * 1e-6f));
    }
    ImGui::End();
  }

  if (!io.WantCaptureMouse) {
    scene.GetCamera().UpdateFov(-io.MouseWheel);
    if (io.MouseWheel) {
      reset_accumulation_ = true;
    }
  }

  ImGui::Render();

  reset_accumulation_ |= rebuild_object_infos_;
}

void App::UpdateDynamicBuffer() {
  GlobalUniformObject global_uniform_object{};
  global_uniform_object.projection =
      renderer_->GetScene().GetCamera().GetProjectionMatrix(
          float(core_->GetFramebufferWidth()) /
              float(core_->GetFramebufferHeight()),
          0.1f, 30.0f);
  global_uniform_object.camera =
      glm::inverse(renderer_->GetScene().GetCameraToWorld());
  global_uniform_object.envmap_id = renderer_->GetScene().GetEnvmapId();
  global_uniform_object.envmap_offset = renderer_->GetScene().GetEnvmapOffset();
  global_uniform_object.hover_id = hover_entity_id_;
  global_uniform_object.selected_id = selected_entity_id_;
  global_uniform_object.envmap_light_direction =
      renderer_->GetScene().GetEnvmapLightDirection();
  global_uniform_object.envmap_minor_color =
      renderer_->GetScene().GetEnvmapMinorColor();
  global_uniform_object.envmap_major_color =
      renderer_->GetScene().GetEnvmapMajorColor();
  global_uniform_object.accumulated_sample = accumulated_sample_;
  global_uniform_object.num_samples =
      renderer_->GetRendererSettings().num_samples;
  global_uniform_object.num_bounces =
      renderer_->GetRendererSettings().num_bounces;
  global_uniform_object.num_objects =
      renderer_->GetScene().GetEntities().size();
  global_uniform_object.enable_mis =
      renderer_->GetRendererSettings().enable_mis;
  global_uniform_object.enable_alpha_shadow =
      renderer_->GetRendererSettings().enable_alpha_shadow;
  global_uniform_object.total_power = total_power_;
  global_uniform_object.total_envmap_power =
      renderer_->GetScene().GetEnvmapTotalPower();
  global_uniform_object.output_selection =
      renderer_->GetRendererSettings().output_selection;

  auto &camera = renderer_->GetScene().GetCamera();
  global_uniform_object.fov = camera.GetFov();
  global_uniform_object.aperture = camera.GetAperture();
  global_uniform_object.focal_length = camera.GetFocalLength();
  global_uniform_object.clamp = camera.GetClamp();
  global_uniform_object.gamma = camera.GetGamma();
  global_uniform_object.aspect = float(core_->GetFramebufferWidth()) /
                                 float(core_->GetFramebufferHeight());
  global_uniform_object.envmap_scale =
      renderer_->GetRendererSettings().envmap_scale;

  global_uniform_buffer_->operator[](0) = global_uniform_object;
  global_uniform_object.projection =
      renderer_->GetScene().GetCamera().GetProjectionMatrix(
          float(core_->GetFramebufferWidth()) /
              float(core_->GetFramebufferHeight()),
          30.0f, 10000.0f);
  global_uniform_buffer_far_->operator[](0) = global_uniform_object;

  UpdateObjectInfo();
}

void App::UpdateHostStencilBuffer() {
  double dx, dy;
  glfwGetCursorPos(core_->GetWindow(), &dx, &dy);
  dx *=
      ((double)core_->GetFramebufferWidth() / (double)core_->GetWindowWidth());
  dy *= ((double)core_->GetFramebufferHeight() /
         (double)core_->GetWindowHeight());
  int x = std::lround(dx), y = std::lround(dy);
  int index = y * core_->GetFramebufferWidth() + x;
  cursor_x_ = x;
  cursor_y_ = y;
  if (x < 0 || x >= core_->GetFramebufferWidth() || y < 0 ||
      y >= core_->GetFramebufferHeight()) {
    hover_entity_id_ = -1;
    hovering_pixel_color_ = glm::vec4{0.0f};
  } else {
    hover_entity_id_ = *reinterpret_cast<int *>(
        stencil_device_buffer_->Map(sizeof(int), index * sizeof(int)));
    stencil_device_buffer_->Unmap();
    hovering_pixel_color_ =
        *reinterpret_cast<glm::vec4 *>(render_frame_device_buffer_->Map(
            sizeof(glm::vec4), index * sizeof(glm::vec4)));
    render_frame_device_buffer_->Unmap();
  }
}

void App::UpdateDeviceAssets() {
  auto &entities = renderer_->GetScene().GetEntities();
  bool rebuild_rt_assets = false;
  for (int i = num_loaded_device_assets_; i < entities.size(); i++) {
    auto &entity = entities[i];
    auto vertices = entity.GetModel()->GetVertices();
    auto indices = entity.GetModel()->GetIndices();
    EntityDeviceAsset device_asset{
        std::make_unique<vulkan::framework::StaticBuffer<Vertex>>(
            core_.get(), vertices.size()),
        std::make_unique<vulkan::framework::StaticBuffer<uint32_t>>(
            core_.get(), indices.size())};
    device_asset.vertex_buffer->Upload(vertices.data());
    device_asset.index_buffer->Upload(indices.data());
    entity_device_assets_.push_back(std::move(device_asset));

    if (app_settings_.hardware_renderer) {
      rebuild_rt_assets = true;
      bottom_level_acceleration_structures_.push_back(
          std::make_unique<
              vulkan::raytracing::BottomLevelAccelerationStructure>(
              core_->GetDevice(), core_->GetCommandPool(), vertices, indices));
      object_info_data_.push_back({uint32_t(ray_tracing_vertex_data_.size()),
                                   uint32_t(ray_tracing_index_data_.size())});
      ray_tracing_vertex_data_.insert(ray_tracing_vertex_data_.end(),
                                      vertices.begin(), vertices.end());
      ray_tracing_index_data_.insert(ray_tracing_index_data_.end(),
                                     indices.begin(), indices.end());
    }
  }
  num_loaded_device_assets_ = int(entities.size());

  if (rebuild_rt_assets) {
    std::vector<std::pair<
        vulkan::raytracing::BottomLevelAccelerationStructure *, glm::mat4>>
        object_instances;
    for (int i = 0; i < entities.size(); i++) {
      auto &entity = entities[i];
      object_instances.emplace_back(
          bottom_level_acceleration_structures_[i].get(),
          entity.GetTransformMatrix());
    }
    top_level_acceleration_structure_ =
        std::make_unique<vulkan::raytracing::TopLevelAccelerationStructure>(
            core_->GetDevice(), core_->GetCommandPool(), object_instances);
    ray_tracing_vertex_buffer_ =
        std::make_unique<vulkan::framework::StaticBuffer<Vertex>>(
            core_.get(), ray_tracing_vertex_data_.size());
    ray_tracing_vertex_buffer_->Upload(ray_tracing_vertex_data_.data());
    ray_tracing_index_buffer_ =
        std::make_unique<vulkan::framework::StaticBuffer<uint32_t>>(
            core_.get(), ray_tracing_index_data_.size());
    ray_tracing_index_buffer_->Upload(ray_tracing_index_data_.data());
    object_info_buffer_ =
        std::make_unique<vulkan::framework::StaticBuffer<ObjectInfo>>(
            core_.get(), object_info_data_.size());
    object_info_buffer_->Upload(object_info_data_.data());

    rebuild_ray_tracing_pipeline_ = true;
  }

  auto &textures = renderer_->GetScene().GetTextures();
  if (num_loaded_device_textures_ != textures.size()) {
    for (int i = num_loaded_device_textures_; i < textures.size(); i++) {
      auto &texture = textures[i];
      VkFilter filter{VK_FILTER_LINEAR};
      switch (texture.GetSampleType()) {
        case SAMPLE_TYPE_LINEAR:
          filter = VK_FILTER_LINEAR;
          break;
        case SAMPLE_TYPE_NEAREST:
          filter = VK_FILTER_NEAREST;
          break;
      }
      device_texture_samplers_.emplace_back(
          std::make_unique<vulkan::framework::TextureImage>(
              core_.get(), texture.GetWidth(), texture.GetHeight()),
          std::make_unique<vulkan::Sampler>(core_->GetDevice(), filter,
                                            VK_SAMPLER_ADDRESS_MODE_REPEAT));
      auto &device_texture_sampler = *device_texture_samplers_.rbegin();
      std::unique_ptr<vulkan::Buffer> texture_image_buffer =
          std::make_unique<vulkan::Buffer>(
              core_->GetDevice(),
              texture.GetWidth() * texture.GetHeight() * sizeof(glm::vec4),
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      std::memcpy(texture_image_buffer->Map(), &texture.operator()(0, 0),
                  texture.GetWidth() * texture.GetHeight() * sizeof(glm::vec4));
      vulkan::UploadImage(core_->GetCommandPool(),
                          device_texture_sampler.first->GetImage(),
                          texture_image_buffer.get());
    }
    num_loaded_device_textures_ = int(textures.size());
    rebuild_render_nodes_ = true;
  }
}

void App::HandleImGuiIO() {
  auto &io = ImGui::GetIO();
  if (!io.WantCaptureMouse) {
    if (io.MouseClicked[0]) {
      selected_entity_id_ = hover_entity_id_;
    }
    if (io.MouseClicked[1]) {
      selected_entity_id_ = -1;
    }
  }
}

void App::RebuildRenderNodes() {
  std::vector<std::pair<vulkan::framework::TextureImage *, vulkan::Sampler *>>
      binding_texture_samplers_;
  for (auto &device_texture_sampler : device_texture_samplers_) {
    binding_texture_samplers_.emplace_back(device_texture_sampler.first.get(),
                                           device_texture_sampler.second.get());
  }
  preview_render_node_ =
      std::make_unique<vulkan::framework::RenderNode>(core_.get());
  preview_render_node_->AddShader("../../shaders/scene_view.frag.spv",
                                  VK_SHADER_STAGE_FRAGMENT_BIT);
  preview_render_node_->AddShader("../../shaders/scene_view.vert.spv",
                                  VK_SHADER_STAGE_VERTEX_BIT);
  preview_render_node_->AddUniformBinding(
      global_uniform_buffer_.get(),
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
  preview_render_node_->AddBufferBinding(entity_uniform_buffer_.get(),
                                         VK_SHADER_STAGE_VERTEX_BIT);
  preview_render_node_->AddBufferBinding(
      material_uniform_buffer_.get(),
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
  preview_render_node_->AddUniformBinding(binding_texture_samplers_,
                                          VK_SHADER_STAGE_FRAGMENT_BIT);
  preview_render_node_->AddColorAttachment(render_frame_.get());
  preview_render_node_->AddDepthAttachment(depth_buffer_.get());
  preview_render_node_->AddColorAttachment(
      stencil_buffer_.get(),
      VkPipelineColorBlendAttachmentState{
          VK_FALSE, VK_BLEND_FACTOR_SRC_ALPHA,
          VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
          VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT});
  preview_render_node_->VertexInput(
      {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT,
       VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT,
       VK_FORMAT_R32_SFLOAT});
  preview_render_node_->BuildRenderNode(core_->GetFramebufferWidth(),
                                        core_->GetFramebufferHeight());

  preview_render_node_far_ =
      std::make_unique<vulkan::framework::RenderNode>(core_.get());
  preview_render_node_far_->AddShader("../../shaders/scene_view.frag.spv",
                                      VK_SHADER_STAGE_FRAGMENT_BIT);
  preview_render_node_far_->AddShader("../../shaders/scene_view.vert.spv",
                                      VK_SHADER_STAGE_VERTEX_BIT);
  preview_render_node_far_->AddUniformBinding(
      global_uniform_buffer_far_.get(),
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
  preview_render_node_far_->AddBufferBinding(entity_uniform_buffer_.get(),
                                             VK_SHADER_STAGE_VERTEX_BIT);
  preview_render_node_far_->AddBufferBinding(
      material_uniform_buffer_.get(),
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
  preview_render_node_far_->AddUniformBinding(binding_texture_samplers_,
                                              VK_SHADER_STAGE_FRAGMENT_BIT);
  preview_render_node_far_->AddColorAttachment(render_frame_.get());
  preview_render_node_far_->AddDepthAttachment(depth_buffer_.get());
  preview_render_node_far_->AddColorAttachment(
      stencil_buffer_.get(),
      VkPipelineColorBlendAttachmentState{
          VK_FALSE, VK_BLEND_FACTOR_SRC_ALPHA,
          VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
          VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT});
  preview_render_node_far_->VertexInput(
      {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT,
       VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT,
       VK_FORMAT_R32_SFLOAT});
  preview_render_node_far_->BuildRenderNode(core_->GetFramebufferWidth(),
                                            core_->GetFramebufferHeight());

  envmap_render_node_ =
      std::make_unique<vulkan::framework::RenderNode>(core_.get());
  envmap_render_node_->AddShader("../../shaders/envmap.frag.spv",
                                 VK_SHADER_STAGE_FRAGMENT_BIT);
  envmap_render_node_->AddShader("../../shaders/envmap.vert.spv",
                                 VK_SHADER_STAGE_VERTEX_BIT);
  envmap_render_node_->AddUniformBinding(global_uniform_buffer_.get(),
                                         VK_SHADER_STAGE_FRAGMENT_BIT);
  envmap_render_node_->AddUniformBinding(binding_texture_samplers_,
                                         VK_SHADER_STAGE_FRAGMENT_BIT);
  envmap_render_node_->AddColorAttachment(render_frame_.get());
  envmap_render_node_->VertexInput({VK_FORMAT_R32G32_SFLOAT});
  envmap_render_node_->BuildRenderNode(core_->GetFramebufferWidth(),
                                       core_->GetFramebufferHeight());

  postproc_render_node_ =
      std::make_unique<vulkan::framework::RenderNode>(core_.get());
  postproc_render_node_->AddShader("../../shaders/postproc.frag.spv",
                                   VK_SHADER_STAGE_FRAGMENT_BIT);
  postproc_render_node_->AddShader("../../shaders/postproc.vert.spv",
                                   VK_SHADER_STAGE_VERTEX_BIT);
  postproc_render_node_->AddUniformBinding(global_uniform_buffer_.get(),
                                           VK_SHADER_STAGE_FRAGMENT_BIT);
  postproc_render_node_->AddUniformBinding(stencil_buffer_.get(),
                                           VK_SHADER_STAGE_FRAGMENT_BIT);
  postproc_render_node_->AddUniformBinding(
      render_frame_.get(), linear_sampler_.get(), VK_SHADER_STAGE_FRAGMENT_BIT);
  postproc_render_node_->AddColorAttachment(screen_frame_.get());
  postproc_render_node_->VertexInput({VK_FORMAT_R32G32_SFLOAT});
  postproc_render_node_->BuildRenderNode(core_->GetFramebufferWidth(),
                                         core_->GetFramebufferHeight());

  host_result_render_node_ =
      std::make_unique<vulkan::framework::RenderNode>(core_.get());
  host_result_render_node_->AddShader("../../shaders/host_result.vert.spv",
                                      VK_SHADER_STAGE_VERTEX_BIT);
  host_result_render_node_->AddShader("../../shaders/host_result.frag.spv",
                                      VK_SHADER_STAGE_FRAGMENT_BIT);
  host_result_render_node_->AddUniformBinding(accumulation_number_.get(),
                                              VK_SHADER_STAGE_FRAGMENT_BIT);
  host_result_render_node_->AddUniformBinding(accumulation_color_.get(),
                                              VK_SHADER_STAGE_FRAGMENT_BIT);
  host_result_render_node_->AddColorAttachment(render_frame_.get());
  host_result_render_node_->VertexInput({VK_FORMAT_R32G32_SFLOAT});
  host_result_render_node_->BuildRenderNode(core_->GetFramebufferWidth(),
                                            core_->GetFramebufferHeight());

  BuildRayTracingPipeline();
}

bool App::UpdateImGuizmo() {
  bool value_changed = false;
  ImGui::Text("Gizmo");
  ImGui::Separator();

  static ImGuizmo::OPERATION current_guizmo_operation(ImGuizmo::ROTATE);
  static ImGuizmo::MODE current_guizmo_mode(ImGuizmo::WORLD);
  if (ImGui::IsKeyPressed(ImGuiKey_T)) {
    current_guizmo_operation = ImGuizmo::TRANSLATE;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_R)) {
    current_guizmo_operation = ImGuizmo::ROTATE;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_S)) {
    current_guizmo_operation = ImGuizmo::SCALE;
  }
  float matrixTranslation[3], matrixRotation[3], matrixScale[3];
  auto &scene = renderer_->GetScene();
  auto &matrix = scene.GetEntity(selected_entity_id_).GetTransformMatrix();
  auto &io = ImGui::GetIO();

  if (ImGui::RadioButton("Translate",
                         current_guizmo_operation == ImGuizmo::TRANSLATE)) {
    current_guizmo_operation = ImGuizmo::TRANSLATE;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Rotate",
                         current_guizmo_operation == ImGuizmo::ROTATE)) {
    current_guizmo_operation = ImGuizmo::ROTATE;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Scale",
                         current_guizmo_operation == ImGuizmo::SCALE)) {
    current_guizmo_operation = ImGuizmo::SCALE;
  }
  ImGuizmo::DecomposeMatrixToComponents(reinterpret_cast<float *>(&matrix),
                                        matrixTranslation, matrixRotation,
                                        matrixScale);
  value_changed |= ImGui::InputFloat3("Translation", matrixTranslation);
  value_changed |= ImGui::InputFloat3("Rotation", matrixRotation);
  value_changed |= ImGui::InputFloat3("Scale", matrixScale);
  ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation,
                                          matrixScale,
                                          reinterpret_cast<float *>(&matrix));

  if (current_guizmo_operation != ImGuizmo::SCALE) {
    if (ImGui::RadioButton("Local", current_guizmo_mode == ImGuizmo::LOCAL)) {
      current_guizmo_mode = ImGuizmo::LOCAL;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("World", current_guizmo_mode == ImGuizmo::WORLD)) {
      current_guizmo_mode = ImGuizmo::WORLD;
    }
  }

  glm::mat4 imguizmo_view_ = glm::inverse(scene.GetCameraToWorld());
  glm::mat4 imguizmo_proj_ =
      glm::scale(glm::mat4{1.0f}, glm::vec3{1.0f, -1.0f, 1.0f}) *
      scene.GetCamera().GetProjectionMatrix(
          float(io.DisplaySize.x) / float(io.DisplaySize.y), 0.1f, 10000.0f);
  ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
  value_changed |= ImGuizmo::Manipulate(
      reinterpret_cast<float *>(&imguizmo_view_),
      reinterpret_cast<float *>(&imguizmo_proj_), current_guizmo_operation,
      current_guizmo_mode, reinterpret_cast<float *>(&matrix), nullptr,
      nullptr);
  return value_changed;
}

void App::UpdateCamera() {
  double posx, posy;
  glfwGetCursorPos(core_->GetWindow(), &posx, &posy);
  glm::vec2 current_pos = {posx, posy};
  static glm::vec2 last_pos = current_pos;
  auto diff = current_pos - last_pos;
  last_pos = current_pos;

  auto current_time_point = std::chrono::steady_clock::now();
  static auto last_time_point = current_time_point;
  auto duration =
      (current_time_point - last_time_point) / std::chrono::milliseconds(1);
  last_time_point += duration * std::chrono::milliseconds(1);

  auto &scene = renderer_->GetScene();
  auto camera = scene.GetCameraToWorld();
  auto R = glm::mat3{camera};
  auto x = glm::vec3{camera[0]};
  auto y = glm::vec3{camera[1]};
  auto z = glm::vec3{camera[2]};
  auto &position = scene.GetCameraPosition();
  auto &pitch_yaw_roll = scene.GetCameraPitchYawRoll();

  auto &io = ImGui::GetIO();

  auto speed = scene.GetCameraSpeed();
  if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) {
    if (ImGui::IsKeyDown(ImGuiKey_W)) {
      position += duration * 0.001f * (-z) * speed;
      reset_accumulation_ = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_S)) {
      position += duration * 0.001f * (z)*speed;
      reset_accumulation_ = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_A)) {
      position += duration * 0.001f * (-x) * speed;
      reset_accumulation_ = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_D)) {
      position += duration * 0.001f * (x)*speed;
      reset_accumulation_ = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
      position += duration * 0.001f * (-y) * speed;
      reset_accumulation_ = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_Space)) {
      position += duration * 0.001f * (y)*speed;
      reset_accumulation_ = true;
    }
  }

  auto rotation_scale = 1.0f / float(core_->GetWindowHeight());
  if (!io.WantCaptureMouse) {
    if (ImGui::IsKeyDown(ImGuiKey_MouseLeft)) {
      pitch_yaw_roll.x -= diff.y * rotation_scale;
      pitch_yaw_roll.y -= diff.x * rotation_scale;
      if (diff != glm::vec2{0.0f}) {
        reset_accumulation_ = true;
      }
    }
  }
  pitch_yaw_roll.x = glm::clamp(pitch_yaw_roll.x, -glm::pi<float>() * 0.5f,
                                glm::pi<float>() * 0.5f);
  pitch_yaw_roll.y = glm::mod(pitch_yaw_roll.y, glm::pi<float>() * 2.0f);
  pitch_yaw_roll.z = glm::mod(pitch_yaw_roll.z, glm::pi<float>() * 2.0f);
}

void App::UploadAccumulationResult() {
  if (app_settings_.hardware_renderer) {
  } else {
    renderer_->RetrieveAccumulationResult(
        reinterpret_cast<glm::vec4 *>(host_accumulation_color_->Map()),
        reinterpret_cast<float *>(host_accumulation_number_->Map()));
    host_accumulation_number_->Unmap();
    host_accumulation_color_->Unmap();
    vulkan::UploadImage(core_->GetCommandPool(),
                        accumulation_color_->GetImage(),
                        host_accumulation_color_.get());
    vulkan::UploadImage(core_->GetCommandPool(),
                        accumulation_number_->GetImage(),
                        host_accumulation_number_.get());
  }
}

void App::UpdateTopLevelAccelerationStructure() {
  std::vector<std::pair<vulkan::raytracing::BottomLevelAccelerationStructure *,
                        glm::mat4>>
      object_instances;
  auto &entities = renderer_->GetScene().GetEntities();
  for (int i = 0; i < entities.size(); i++) {
    auto &entity = entities[i];
    object_instances.emplace_back(
        bottom_level_acceleration_structures_[i].get(),
        entity.GetTransformMatrix());
  }
  top_level_acceleration_structure_->UpdateAccelerationStructure(
      core_->GetCommandPool(), object_instances);
}

void App::BuildRayTracingPipeline() {
  if (!app_settings_.hardware_renderer) {
    return;
  }

  std::vector<std::pair<vulkan::framework::TextureImage *, vulkan::Sampler *>>
      binding_texture_samplers_;
  for (auto &device_texture_sampler : device_texture_samplers_) {
    binding_texture_samplers_.emplace_back(device_texture_sampler.first.get(),
                                           device_texture_sampler.second.get());
  }
  ray_tracing_render_node_ =
      std::make_unique<vulkan::framework::RayTracingRenderNode>(core_.get());
  ray_tracing_render_node_->AddUniformBinding(
      top_level_acceleration_structure_.get(), VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddUniformBinding(accumulation_color_.get(),
                                              VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddUniformBinding(accumulation_number_.get(),
                                              VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddUniformBinding(global_uniform_buffer_.get(),
                                              VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddBufferBinding(entity_uniform_buffer_.get(),
                                             VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddBufferBinding(material_uniform_buffer_.get(),
                                             VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddBufferBinding(object_info_buffer_.get(),
                                             VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddBufferBinding(ray_tracing_vertex_buffer_.get(),
                                             VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddBufferBinding(ray_tracing_index_buffer_.get(),
                                             VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddUniformBinding(binding_texture_samplers_,
                                              VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddBufferBinding(primitive_cdf_buffer_.get(),
                                             VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddBufferBinding(envmap_cdf_buffer_.get(),
                                             VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->AddBufferBinding(sobol_table_buffer_.get(),
                                             VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ray_tracing_render_node_->SetShaders("../../shaders/path_tracing.rgen.spv",
                                       "../../shaders/path_tracing.rmiss.spv",
                                       "../../shaders/path_tracing.rchit.spv");
  auto start_time = std::chrono::steady_clock::now();
  ray_tracing_render_node_->BuildRenderNode();
  auto dur = std::chrono::steady_clock::now() - start_time;
  LAND_INFO("Raytracing pipeline build time: {}ms",
            dur / std::chrono::milliseconds(1));
}

void App::Capture(const std::string &file_path) {
  LAND_INFO("Capture Saving... Path: [{}]", file_path);
  auto write_buffer = [&file_path, this](glm::vec4 *buffer, int width,
                                         int height, float scale) {
    if (absl::EndsWith(file_path, ".hdr")) {
      stbi_write_hdr(file_path.c_str(), width, height, 4,
                     reinterpret_cast<float *>(buffer));
    } else {
      std::vector<uint8_t> buffer24bit(width * height * 3);
      auto float2u8 = [](float v) {
        return uint8_t(std::max(0, std::min(255, int(v * 255.0f))));
      };
      float inv_gamma = 1.0f / renderer_->GetScene().GetCamera().GetGamma();
      for (int i = 0; i < width * height; i++) {
        buffer[i] *= scale;
        buffer24bit[i * 3] = float2u8(pow(buffer[i].r, inv_gamma));
        buffer24bit[i * 3 + 1] = float2u8(pow(buffer[i].g, inv_gamma));
        buffer24bit[i * 3 + 2] = float2u8(pow(buffer[i].b, inv_gamma));
      }
      if (absl::EndsWith(file_path, ".png")) {
        stbi_write_png(file_path.c_str(), width, height, 3, buffer24bit.data(),
                       width * 3);
      } else if (absl::EndsWith(file_path, ".jpg") ||
                 absl::EndsWith(file_path, ".jpeg")) {
        stbi_write_jpg(file_path.c_str(), width, height, 3, buffer24bit.data(),
                       100);
      } else {
        stbi_write_bmp(file_path.c_str(), width, height, 3, buffer24bit.data());
      }
    }
  };

  if (app_settings_.hardware_renderer) {
    auto image = accumulation_color_->GetImage();
    std::vector<glm::vec4> captured_buffer(image->GetWidth() *
                                           image->GetHeight());
    std::unique_ptr<vulkan::Buffer> image_buffer =
        std::make_unique<vulkan::Buffer>(
            core_->GetDevice(),
            sizeof(glm::vec4) * image->GetWidth() * image->GetHeight(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    DownloadImage(core_->GetCommandPool(), image, image_buffer.get());
    std::memcpy(captured_buffer.data(), image_buffer->Map(),
                sizeof(glm::vec4) * image->GetWidth() * image->GetHeight());
    float scale = 1.0f / float(std::max(1u, accumulated_sample_));
    write_buffer(captured_buffer.data(), image->GetWidth(), image->GetHeight(),
                 scale);
  } else {
    auto captured_buffer = renderer_->CaptureRenderedImage();
    write_buffer(captured_buffer.data(), renderer_->GetWidth(),
                 renderer_->GetHeight(), 1.0f);
  }
}

void App::OpenFile(const std::string &path) {
  if (absl::EndsWith(path, ".png") || absl::EndsWith(path, ".jpg") ||
      absl::EndsWith(path, ".bmp") || absl::EndsWith(path, ".hdr") ||
      absl::EndsWith(path, ".jpeg")) {
    renderer_->LoadTexture(path);
  } else if (absl::EndsWith(path, ".obj")) {
    renderer_->LoadObjMesh(path);
    reset_accumulation_ = true;
    rebuild_object_infos_ = true;
  } else if (absl::EndsWith(path, ".xml")) {
    renderer_->LoadScene(path);
    renderer_->ResetAccumulation();
    num_loaded_device_textures_ = 0;
    num_loaded_device_assets_ = 0;
    device_texture_samplers_.clear();
    entity_device_assets_.clear();
    selected_entity_id_ = -1;
    envmap_require_configure_ = true;
    if (app_settings_.hardware_renderer) {
      reset_accumulation_ = true;
      rebuild_object_infos_ = true;
      top_level_acceleration_structure_.reset();
      bottom_level_acceleration_structures_.clear();
      object_info_data_.clear();
      ray_tracing_vertex_data_.clear();
      ray_tracing_index_data_.clear();
    }
  }
}

void App::UpdateObjectInfo() {
  if (rebuild_object_infos_) {
    std::vector<EntityUniformObject> object_sampler_infos;
    std::vector<float> primitive_cdf;
    auto &entities = renderer_->GetScene().GetEntities();

    float total_power = 0.0f;
    for (auto &entity : entities) {
      EntityUniformObject object_sampler_info{};

      auto mat = entity.GetMaterial();
      auto transform = entity.GetTransformMatrix();
      object_sampler_info.object_to_world = transform;
      object_sampler_info.num_primitives = 0;
      object_sampler_info.primitive_offset = primitive_cdf.size();
      object_sampler_info.power = 0.0f;
      if (mat.emission_strength > 1e-4f) {
        auto vertices = entity.GetModel()->GetVertices();
        auto indices = entity.GetModel()->GetIndices();
        object_sampler_info.num_primitives = indices.size() / 3;

        if (object_sampler_info.num_primitives) {
          std::vector<float> object_primitive_cdf;
          for (auto &vertex : vertices) {
            vertex.position = transform * glm::vec4{vertex.position, 1.0f};
          }
          auto total_area = 0.0f;
          object_primitive_cdf.push_back(0.0f);
          for (int i = 0; i < object_sampler_info.num_primitives; i++) {
            auto v0 = vertices[indices[i * 3]];
            auto v1 = vertices[indices[i * 3 + 1]];
            auto v2 = vertices[indices[i * 3 + 2]];
            auto area =
                0.5f * glm::length(glm::cross(v1.position - v0.position,
                                              v2.position - v0.position));
            total_area += area;
            object_primitive_cdf.push_back(total_area);
          }
          object_sampler_info.power = total_area * mat.emission_strength;
          object_sampler_info.area = total_area;
          for (auto &cdf : object_primitive_cdf) {
            cdf /= total_area;
          }
          object_primitive_cdf.push_back(1.0f);
          primitive_cdf.insert(primitive_cdf.end(),
                               object_primitive_cdf.begin(),
                               object_primitive_cdf.end());
        }
      }
      total_power += object_sampler_info.power;
      object_sampler_info.cdf = total_power;
      object_sampler_infos.emplace_back(object_sampler_info);
    }

    if (total_power > 0.0f) {
      for (auto &object_sampler_info : object_sampler_infos) {
        object_sampler_info.pdf = object_sampler_info.power / total_power;
        object_sampler_info.cdf /= total_power;
        if (object_sampler_info.area) {
          object_sampler_info.sample_density =
              object_sampler_info.pdf / object_sampler_info.area;
        } else {
          object_sampler_info.sample_density = 0.0;
        }
      }
    }

    object_sampler_infos.emplace_back();
    total_power_ = total_power;

    if (primitive_cdf.empty()) {
      primitive_cdf.emplace_back();
    }

    std::vector<Material> materials;
    for (auto &entitie : entities) {
      materials.push_back(entitie.GetMaterial());
    }

    {
      material_uniform_buffer_->Resize(entities.size());
      entity_uniform_buffer_->Resize(object_sampler_infos.size());
      primitive_cdf_buffer_->Resize(primitive_cdf.size());

      if (preview_render_node_) {
        preview_render_node_->UpdateDescriptorSetBinding(1);
        preview_render_node_->UpdateDescriptorSetBinding(2);
      }
      if (preview_render_node_far_) {
        preview_render_node_far_->UpdateDescriptorSetBinding(1);
        preview_render_node_far_->UpdateDescriptorSetBinding(2);
      }

      if (ray_tracing_render_node_) {
        ray_tracing_render_node_->UpdateDescriptorSetBinding(4);
        ray_tracing_render_node_->UpdateDescriptorSetBinding(5);
        ray_tracing_render_node_->UpdateDescriptorSetBinding(10);
      }
    }

    entity_uniform_buffer_->Upload(object_sampler_infos.data());
    primitive_cdf_buffer_->Upload(primitive_cdf.data());
    material_uniform_buffer_->Upload(materials.data());

    rebuild_object_infos_ = false;
  }
}

void App::UpdateRenderNodes() {
  if (rebuild_render_nodes_) {
    RebuildRenderNodes();
    rebuild_render_nodes_ = false;
  }
}

}  // namespace sparks
