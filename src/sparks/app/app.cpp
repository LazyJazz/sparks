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
  core_->SetFrameSizeCallback([this](int width, int height) {
    screen_frame_->Resize(width, height);
    depth_buffer_->Resize(width, height);
    render_node_->BuildRenderNode(width, height);
  });

  core_->ImGuiInit(screen_frame_.get(), "../../fonts/NotoSansSC-Regular.otf",
                   24.0f);

  global_uniform_buffer_ =
      std::make_unique<vulkan::framework::DynamicBuffer<GlobalUniformObject>>(
          core_.get(), 1);
  entity_uniform_buffer_ =
      std::make_unique<vulkan::framework::DynamicBuffer<EntityUniformObject>>(
          core_.get(), 16384);
  render_node_ = std::make_unique<vulkan::framework::RenderNode>(core_.get());
  render_node_->AddShader("../../shaders/scene_view.frag.spv",
                          VK_SHADER_STAGE_FRAGMENT_BIT);
  render_node_->AddShader("../../shaders/scene_view.vert.spv",
                          VK_SHADER_STAGE_VERTEX_BIT);
  render_node_->AddUniformBinding(global_uniform_buffer_.get(),
                                  VK_SHADER_STAGE_VERTEX_BIT);
  render_node_->AddBufferBinding(entity_uniform_buffer_.get(),
                                 VK_SHADER_STAGE_VERTEX_BIT);
  render_node_->AddColorAttachment(screen_frame_.get());
  render_node_->AddDepthAttachment(depth_buffer_.get());
  render_node_->VertexInput(
      {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT,
       VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT});
  render_node_->BuildRenderNode(core_->GetFramebufferWidth(),
                                core_->GetFramebufferHeight());

  renderer_->GetScene().AddEntity(Mesh::Sphere(glm::vec3{0.0f, 0.0f, 2.0f}),
                                  Material{}, glm::mat4{1.0f});

  auto &entities = renderer_->GetScene().GetEntities();
  for (int i = 0; i < entities.size(); i++) {
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
    device_asset.vertex_buffer.release();
    device_asset.index_buffer.release();
  }
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
  global_uniform_buffer_->operator[](0).projection =
      renderer_->GetScene().GetCamera().GetProjectionMatrix(
          float(core_->GetFramebufferWidth()) /
          float(core_->GetFramebufferHeight()));
  global_uniform_buffer_->operator[](0).camera =
      glm::inverse(renderer_->GetScene().GetCameraToWorld());
  auto &entities = renderer_->GetScene().GetEntities();
  for (int i = 0; i < entities.size(); i++) {
    auto &entity = entities[i];
    entity_uniform_buffer_->operator[](i).model = entity.GetTransformMatrix();
  }
}

void App::OnRender() {
  core_->BeginCommandRecord();
  screen_frame_->ClearColor({0.0f, 0.0f, 0.0f, 1.0f});
  depth_buffer_->ClearDepth({1.0f, 0});
  for (int i = 0; i < entity_device_assets_.size(); i++) {
    auto &entity_asset = entity_device_assets_[i];
    render_node_->Draw(entity_asset.vertex_buffer.get(),
                       entity_asset.index_buffer.get(),
                       entity_asset.index_buffer->Size(), i);
  }
  core_->ImGuiRender();
  core_->Output(screen_frame_.get());
  core_->EndCommandRecordAndSubmit();
}

void App::OnClose() {
  entity_device_assets_.clear();
  render_node_.reset();
  global_uniform_buffer_.reset();
  entity_uniform_buffer_.reset();
  screen_frame_.reset();
}

void App::UpdateImGui() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  if (global_settings_window_open_) {
    ImGui::Begin("Global Settings", &global_settings_window_open_,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::CollapsingHeader("Camera")) {
      renderer_->GetScene().GetCamera().ImGuiItems();
    }
    ImGui::SetWindowPos({12, 12}, ImGuiCond_Once);
    ImGui::Text("Hello, World!");
    ImGui::Separator();
    ImGui::Text("I'm Sparks!");
    ImGui::End();
  }

  ImGui::Render();
}

}  // namespace sparks
