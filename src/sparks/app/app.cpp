#include "sparks/app/app.h"

#include "iostream"
#include "sparks/util/util.h"

namespace sparks {

App::App(Renderer *renderer, const AppSettings &app_settings) {
  renderer_ = renderer;
  vulkan::framework::CoreSettings core_settings;
  core_settings.window_title = "Sparks";
  core_settings.window_width = app_settings.width;
  core_settings.window_height = app_settings.height;
  core_settings.validation_layer = app_settings.validation_layer;
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
  screen_frame_ = std::make_unique<vulkan::framework::TextureImage>(
      core_.get(), core_->GetFramebufferWidth(), core_->GetFramebufferHeight(),
      VK_FORMAT_B8G8R8A8_UNORM,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  depth_buffer_ = std::make_unique<vulkan::framework::TextureImage>(
      core_.get(), core_->GetFramebufferWidth(), core_->GetFramebufferHeight(),
      VK_FORMAT_D32_SFLOAT,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  stencil_buffer_ = std::make_unique<vulkan::framework::TextureImage>(
      core_.get(), core_->GetFramebufferWidth(), core_->GetFramebufferHeight(),
      VK_FORMAT_R32_UINT,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
  stencil_device_buffer_ = std::make_unique<vulkan::Buffer>(
      core_->GetDevice(),
      sizeof(uint32_t) * core_->GetFramebufferWidth() *
          core_->GetFramebufferHeight(),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  stencil_host_buffer_.resize(core_->GetFramebufferWidth() *
                              core_->GetFramebufferHeight());

  core_->SetFrameSizeCallback([this](int width, int height) {
    core_->GetDevice()->WaitIdle();
    screen_frame_->Resize(width, height);
    depth_buffer_->Resize(width, height);
    stencil_buffer_->Resize(width, height);
    render_node_->BuildRenderNode(width, height);
    envmap_render_node_->BuildRenderNode(width, height);
    stencil_device_buffer_ = std::make_unique<vulkan::Buffer>(
        core_->GetDevice(), sizeof(uint32_t) * width * height,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stencil_host_buffer_.resize(width * height);
  });

  core_->ImGuiInit(screen_frame_.get(), "../../fonts/NotoSansSC-Regular.otf",
                   24.0f);

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
  UpdateDeviceAssets();
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
  UpdateImGui();
  UpdateDynamicBuffer();
  UpdateHostStencilBuffer();
  UpdateDeviceAssets();
  HandleImGuiIO();
}

void App::OnRender() {
  core_->BeginCommandRecord();
  screen_frame_->ClearColor({0.0f, 0.0f, 0.0f, 1.0f});
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
  core_->TemporalSubmit();
  core_->ImGuiRender();
  core_->Output(screen_frame_.get());
  core_->EndCommandRecordAndSubmit();
}

void App::OnClose() {
  entity_device_assets_.clear();
  render_node_.reset();
  stencil_buffer_.reset();
  global_uniform_buffer_.reset();
  entity_uniform_buffer_.reset();
  screen_frame_.reset();
}

void App::UpdateImGui() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  auto &io = ImGui::GetIO();
  auto &scene = renderer_->GetScene();
  if (global_settings_window_open_) {
    ImGui::Begin("Global Settings", &global_settings_window_open_,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetWindowPos({12, 12}, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Camera")) {
      scene.GetCamera().ImGuiItems();
    }
    if (ImGui::CollapsingHeader("Environment Map")) {
      ImGui::SliderAngle("Offset", &scene.GetEnvmapOffset(), 0.0f, 360.0f,
                         "%.0f deg");
    }
    if (selected_entity_id_ != -1) {
      if (ImGui::CollapsingHeader("Material")) {
        Material &material =
            renderer_->GetScene().GetEntity(selected_entity_id_).GetMaterial();
        ImGui::ColorEdit3(
            "Albedo Color", &material.albedo_color[0],
            ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_Float);
      }
    }
    ImGui::Text("Hover item: %s",
                (hover_entity_id_ == -1)
                    ? "None"
                    : std::to_string(hover_entity_id_).c_str());
    ImGui::Text("Selected item: %s",
                (selected_entity_id_ == -1)
                    ? "None"
                    : std::to_string(selected_entity_id_).c_str());
    ImGui::Text("Want Capture Mouse: %s", io.WantCaptureMouse ? "yes" : "no");
    ImGui::Text("Has Mouse Double Clicked: %s",
                io.MouseClicked[0] ? "yes" : "no");
    ImGui::End();
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
  } else {
    hover_entity_id_ = *reinterpret_cast<int *>(
        stencil_device_buffer_->Map(sizeof(int), index * sizeof(int)));
    stencil_device_buffer_->Unmap();
  }
}

void App::UpdateDeviceAssets() {
  auto &entities = renderer_->GetScene().GetEntities();
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
  }
  num_loaded_device_assets_ = int(entities.size());

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
  render_node_->AddUniformBinding(global_uniform_buffer_.get(),
                                  VK_SHADER_STAGE_VERTEX_BIT);
  render_node_->AddBufferBinding(entity_uniform_buffer_.get(),
                                 VK_SHADER_STAGE_VERTEX_BIT);
  render_node_->AddBufferBinding(
      material_uniform_buffer_.get(),
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
  render_node_->AddUniformBinding(binding_texture_samplers_,
                                  VK_SHADER_STAGE_FRAGMENT_BIT);
  render_node_->AddColorAttachment(screen_frame_.get());
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
  envmap_render_node_->AddColorAttachment(screen_frame_.get());
  envmap_render_node_->VertexInput({VK_FORMAT_R32G32_SFLOAT});
  envmap_render_node_->BuildRenderNode(core_->GetFramebufferWidth(),
                                       core_->GetFramebufferHeight());
}

}  // namespace sparks
