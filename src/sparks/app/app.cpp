#include "sparks/app/app.h"

namespace sparks {

App::App() {
  vulkan::framework::CoreSettings core_settings;
  core_settings.window_title = "Sparks";
  core_settings.window_width = 1920;
  core_settings.window_height = 1080;
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
  core_->SetFrameSizeCallback(
      [this](int width, int height) { screen_frame_->Resize(width, height); });

  core_->ImGuiInit(screen_frame_.get(), "../../fonts/NotoSansSC-Regular.otf",
                   24.0f);
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
}

void App::OnRender() {
  core_->BeginCommandRecord();
  screen_frame_->ClearColor({0.0f, 0.0f, 0.0f, 1.0f});
  core_->ImGuiRender();
  core_->Output(screen_frame_.get());
  core_->EndCommandRecordAndSubmit();
}

void App::OnClose() {
  screen_frame_.reset();
}

void App::UpdateImGui() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  static bool global_settings_window_open = true;

  if (global_settings_window_open) {
    ImGui::Begin("Global Settings", &global_settings_window_open,
                 ImGuiWindowFlags_NoMove);
    ImGui::SetWindowPos({12, 12}, ImGuiCond_Once);
    ImGui::SetWindowSize({200, 200}, ImGuiCond_Once);
    ImGui::Text("Hello, World!");
    ImGui::Separator();
    ImGui::Text("I'm Sparks!");
    ImGui::End();
  }

  ImGui::Render();
}

}  // namespace sparks
