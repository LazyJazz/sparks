#include "sparks/app/app.h"

#include "ImGuizmo.h"
#include "absl/strings/match.h"
#include "cmath"
#include "glm/gtc/matrix_transform.hpp"
#include "iostream"
#include "sparks/util/util.h"

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
    OnLoop();
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
    screen_frame_->Resize(width, height);
    render_frame_->Resize(width, height);
    depth_buffer_->Resize(width, height);
    stencil_buffer_->Resize(width, height);
    render_node_->BuildRenderNode(width, height);
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
      if (absl::EndsWith(path, ".png") || absl::EndsWith(path, ".jpg") ||
          absl::EndsWith(path, ".bmp") || absl::EndsWith(path, ".hdr") ||
          absl::EndsWith(path, ".jpeg")) {
        renderer_->LoadTexture(path);
      } else if (absl::EndsWith(path, ".obj")) {
        renderer_->LoadObjMesh(path);
      }
    }
  });

  LAND_INFO("Initializing ImGui.");
  core_->ImGuiInit(screen_frame_.get(), "../../fonts/NotoSansSC-Regular.otf",
                   24.0f);

  LAND_INFO("Allocating visual pipeline buffers.");
  global_uniform_buffer_ =
      std::make_unique<vulkan::framework::DynamicBuffer<GlobalUniformObject>>(
          core_.get(), 1);
  entity_uniform_buffer_ =
      std::make_unique<vulkan::framework::DynamicBuffer<EntityUniformObject>>(
          core_.get(), 16384);
  material_uniform_buffer_ =
      std::make_unique<vulkan::framework::DynamicBuffer<Material>>(core_.get(),
                                                                   16384);

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

  ImGuizmo::Enable(true);
  LAND_INFO("Updating device assets.");
  UpdateDeviceAssets();
  LAND_INFO("Building render nodes.");
  RebuildRenderNode();
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
  if (envmap_require_configure_) {
    renderer_->SafeOperation<void>([&]() {
      renderer_->GetScene().UpdateEnvmapConfiguration();
      renderer_->ResetAccumulation();
      envmap_require_configure_ = false;
    });
  }
  UpdateImGui();
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
  render_node_->BeginDraw();
  for (int i = 0; i < entity_device_assets_.size(); i++) {
    auto &entity_asset = entity_device_assets_[i];
    render_node_->DrawDirect(entity_asset.vertex_buffer.get(),
                             entity_asset.index_buffer.get(),
                             entity_asset.index_buffer->Size(), i);
  }
  render_node_->EndDraw();
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
  render_node_.reset();
  stencil_buffer_.reset();
  global_uniform_buffer_.reset();
  entity_uniform_buffer_.reset();
  screen_frame_.reset();
  if (app_settings_.hardware_renderer) {
  } else {
    renderer_->StopWorkers();
  }
}

void App::UpdateImGui() {
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
                     ImGuiWindowFlags_NoResize);
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

    if (app_settings_.hardware_renderer) {
      reset_accumulation_ |= ImGui::SliderInt(
          "Samples", &renderer_->GetRendererSettings().num_samples, 1, 128);
    } else {
      reset_accumulation_ |= ImGui::SliderInt(
          "Samples", &renderer_->GetRendererSettings().num_samples, 1, 16);
    }
    reset_accumulation_ |= ImGui::SliderInt(
        "Bounces", &renderer_->GetRendererSettings().num_bounces, 1, 128);

    scene.EntityCombo("Selected Entity", &selected_entity_id_);

    ImGui::NewLine();
    ImGui::Text("Camera");
    ImGui::Separator();
    reset_accumulation_ |= scene.GetCamera().ImGuiItems();
    reset_accumulation_ |= ImGui::InputFloat3(
        "Position", reinterpret_cast<float *>(&scene.GetCameraPosition()));
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

    if (selected_entity_id_ != -1) {
      ImGui::NewLine();
      ImGui::Text("Material");
      ImGui::Separator();
      static int current_item = 0;
      std::vector<const char *> material_types = {"Lambertian", "Specular",
                                                  "Transmissive", "Principled"};
      Material &material = scene.GetEntity(selected_entity_id_).GetMaterial();
      reset_accumulation_ |=
          ImGui::Combo("Type", reinterpret_cast<int *>(&material.material_type),
                       material_types.data(), material_types.size());
      reset_accumulation_ |= ImGui::ColorEdit3(
          "Albedo Color", &material.albedo_color[0],
          ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_Float);
      reset_accumulation_ |=
          scene.TextureCombo("Albedo Texture", &material.albedo_texture_id);
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
      reset_accumulation_ |= UpdateImGuizmo();
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
    uint32_t current_sample;
    if (app_settings_.hardware_renderer) {
      current_sample = accumulated_sample_;
    } else {
      current_sample = renderer_->GetAccumulatedSamples();
    }
    auto current_time = std::chrono::steady_clock::now();
    static auto last_sample = current_sample;
    static auto last_sample_time = current_time;
    static float sample_rate = 0.0f;
    float duration_us;
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
    ImGui::Text("R:%f G:%f B:%f", hovering_pixel_color_.x,
                hovering_pixel_color_.y, hovering_pixel_color_.z);
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
}

void App::UpdateDynamicBuffer() {
  GlobalUniformObject guo{};
  guo.projection = renderer_->GetScene().GetCamera().GetProjectionMatrix(
      float(core_->GetFramebufferWidth()) /
      float(core_->GetFramebufferHeight()));
  guo.camera = glm::inverse(renderer_->GetScene().GetCameraToWorld());
  guo.envmap_id = renderer_->GetScene().GetEnvmapId();
  guo.envmap_offset = renderer_->GetScene().GetEnvmapOffset();
  guo.hover_id = hover_entity_id_;
  guo.selected_id = selected_entity_id_;
  guo.envmap_light_direction = renderer_->GetScene().GetEnvmapLightDirection();
  guo.envmap_minor_color = renderer_->GetScene().GetEnvmapMinorColor();
  guo.envmap_major_color = renderer_->GetScene().GetEnvmapMajorColor();
  guo.accumulated_sample = accumulated_sample_;
  guo.num_samples = renderer_->GetRendererSettings().num_samples;
  guo.num_bounces = renderer_->GetRendererSettings().num_bounces;
  global_uniform_buffer_->operator[](0) = guo;
  auto &entities = renderer_->GetScene().GetEntities();
  for (int i = 0; i < entities.size(); i++) {
    auto &entity = entities[i];
    entity_uniform_buffer_->operator[](i).model = entity.GetTransformMatrix();
    material_uniform_buffer_->operator[](i) = entity.GetMaterial();
  }
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
    RebuildRenderNode();
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

void App::RebuildRenderNode() {
  std::vector<std::pair<vulkan::framework::TextureImage *, vulkan::Sampler *>>
      binding_texture_samplers_;
  for (auto &device_texture_sampler : device_texture_samplers_) {
    binding_texture_samplers_.emplace_back(device_texture_sampler.first.get(),
                                           device_texture_sampler.second.get());
  }
  render_node_ = std::make_unique<vulkan::framework::RenderNode>(core_.get());
  render_node_->AddShader("../../shaders/scene_view.frag.spv",
                          VK_SHADER_STAGE_FRAGMENT_BIT);
  render_node_->AddShader("../../shaders/scene_view.vert.spv",
                          VK_SHADER_STAGE_VERTEX_BIT);
  render_node_->AddUniformBinding(
      global_uniform_buffer_.get(),
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
  render_node_->AddBufferBinding(entity_uniform_buffer_.get(),
                                 VK_SHADER_STAGE_VERTEX_BIT);
  render_node_->AddBufferBinding(
      material_uniform_buffer_.get(),
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
  render_node_->AddUniformBinding(binding_texture_samplers_,
                                  VK_SHADER_STAGE_FRAGMENT_BIT);
  render_node_->AddColorAttachment(render_frame_.get());
  render_node_->AddDepthAttachment(depth_buffer_.get());
  render_node_->AddColorAttachment(
      stencil_buffer_.get(),
      VkPipelineColorBlendAttachmentState{
          VK_FALSE, VK_BLEND_FACTOR_SRC_ALPHA,
          VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
          VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT});
  render_node_->VertexInput(
      {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT,
       VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT});
  render_node_->BuildRenderNode(core_->GetFramebufferWidth(),
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
      scene.GetCamera().GetProjectionMatrix(float(io.DisplaySize.x) /
                                            float(io.DisplaySize.y));
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

  auto speed = 3.0f;
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
  ray_tracing_render_node_->SetShaders("../../shaders/path_tracing.rgen.spv",
                                       "../../shaders/path_tracing.rmiss.spv",
                                       "../../shaders/path_tracing.rchit.spv");
  ray_tracing_render_node_->BuildRenderNode();
}

}  // namespace sparks
