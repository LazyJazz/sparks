#include "sparks/renderer/renderer.h"

#include "algorithm"
#include "random"

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

void Renderer::StartWorkerThreads() {
  uint32_t num_threads = std::thread::hardware_concurrency() - 2u;
  num_threads = std::max(num_threads, 1u);
  for (int i = 0; i < num_threads; i++) {
    worker_threads_.emplace_back(&Renderer::WorkerThread, this);
  }
}

void Renderer::PauseWorkers() {
  std::unique_lock<std::mutex> lock(task_queue_mutex_);
  render_state_signal_ = RENDER_STATE_SIGNAL_PAUSE;
  wait_for_queue_cv_.notify_all();
  if (num_paused_thread_ != worker_threads_.size()) {
    wait_for_all_pause_.wait(lock);
  }
}

void Renderer::ResumeWorkers() {
  std::unique_lock<std::mutex> lock(task_queue_mutex_);
  render_state_signal_ = RENDER_STATE_SIGNAL_RUN;
  wait_for_resume_cv_.notify_all();
  wait_for_queue_cv_.notify_all();
}

void Renderer::StopWorkers() {
  std::unique_lock<std::mutex> lock(task_queue_mutex_);
  render_state_signal_ = RENDER_STATE_SIGNAL_EXIT;
  wait_for_resume_cv_.notify_all();
  wait_for_queue_cv_.notify_all();
  if (num_exited_thread_ != worker_threads_.size()) {
    wait_for_all_exit_.wait(lock);
  }
  for (auto &worker : worker_threads_) {
    worker.join();
  }
}

void Renderer::WorkerThread() {
  LAND_TRACE("Worker thread started.");
  TaskInfo my_task{};
  std::unique_lock<std::mutex> lock(task_queue_mutex_);
  lock.unlock();
  std::vector<glm::vec3> sample_result;
  while (true) {
    lock.lock();
    while (true) {
      if (render_state_signal_ == RENDER_STATE_SIGNAL_RUN) {
        if (task_queue_.empty()) {
          LAND_TRACE("Wait for task.");
          wait_for_queue_cv_.wait(lock);
        } else {
          my_task = task_queue_.front();
          task_queue_.pop();
          task_queue_.push(my_task);
          break;
        }
      } else if (render_state_signal_ == RENDER_STATE_SIGNAL_PAUSE) {
        num_paused_thread_++;
        if (num_paused_thread_ == worker_threads_.size()) {
          wait_for_all_pause_.notify_all();
        }
        wait_for_resume_cv_.wait(lock);
        num_paused_thread_--;
      } else {
        num_exited_thread_++;
        if (num_exited_thread_ == worker_threads_.size()) {
          wait_for_all_exit_.notify_all();
        }
        LAND_TRACE("Worker thread exited.");
        return;
      }
    }
    lock.unlock();

    sample_result.resize(my_task.width * my_task.height);

    for (uint32_t i = 0; i < my_task.height; i++) {
      for (uint32_t j = 0; j < my_task.width; j++) {
        uint32_t id = i * my_task.width + j;
        uint32_t x = j + my_task.x;
        uint32_t y = i + my_task.y;
        RayGeneration(int(x), int(y), sample_result[id]);
      }
    }

    lock.lock();
    for (uint32_t i = 0; i < my_task.height; i++) {
      for (uint32_t j = 0; j < my_task.width; j++) {
        uint32_t id = i * my_task.width + j;
        uint32_t accumulation_id = (my_task.y + i) * width_ + (my_task.x + j);
        accumulation_color_[accumulation_id] +=
            glm::vec4{sample_result[id], 1.0f};
        accumulation_number_[accumulation_id] += 1.0f;
      }
    }
    lock.unlock();
  }
}

RenderStateSignal Renderer::GetRenderStateSignal() const {
  return render_state_signal_;
}

void Renderer::Resize(uint32_t width, uint32_t height) {
  PauseWorkers();
  width_ = width;
  height_ = height;
  accumulation_number_.resize(width_ * height_);
  accumulation_color_.resize(width_ * height_);
  std::memset(accumulation_number_.data(), 0,
              sizeof(float) * accumulation_number_.size());
  std::memset(accumulation_color_.data(), 0,
              sizeof(glm::vec4) * accumulation_color_.size());
  while (!task_queue_.empty()) {
    task_queue_.pop();
  }
  const uint32_t task_width = 4;
  const uint32_t task_height = 4;
  std::vector<TaskInfo> task_list;
  for (int i = 0; i < (width_ + task_width - 1) / task_width; i++) {
    for (int j = 0; j < (height_ + task_height - 1) / task_height; j++) {
      TaskInfo task_info{};
      task_info.x = i * task_width;
      task_info.y = j * task_height;
      task_info.width = std::min(task_width, width_ - task_info.x);
      task_info.height = std::min(task_height, height_ - task_info.y);
      task_list.push_back(task_info);
    }
  }
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(task_list.begin(), task_list.end(), g);
  for (auto &task : task_list) {
    task_queue_.push(task);
  }
  ResumeWorkers();
}

void Renderer::ResetAccumulation() {
  PauseWorkers();
  std::memset(accumulation_number_.data(), 0,
              sizeof(float) * accumulation_number_.size());
  std::memset(accumulation_color_.data(), 0,
              sizeof(glm::vec4) * accumulation_color_.size());
  ResumeWorkers();
}

void Renderer::RayGeneration(int x, int y, glm::vec3 &color_result) const {
  glm::vec2 pos{(float(x) + 0.5f) / float(width_),
                (float(y) + 0.5f) / float(height_)};
  glm::vec2 range_low{float(x) / float(width_), float(y) / float(height_)};
  glm::vec2 range_high{(float(x) + 1.0f) / float(width_),
                       (float(y) + 1.0f) / float(height_)};
  glm::vec3 origin, direction;

  static int sample = 0;
  std::mt19937 rd(x ^ y + sample);

  scene_.GetCamera().GenerateRay(
      float(width_) / float(height_), range_low, range_high, origin, direction,
      std::uniform_real_distribution<float>(0.0f, 1.0f)(rd),
      std::uniform_real_distribution<float>(0.0f, 1.0f)(rd));
  auto camera_to_world = scene_.GetCameraToWorld();
  origin = camera_to_world * glm::vec4(origin, 1.0f);
  direction = camera_to_world * glm::vec4(direction, 0.0f);
  color_result = SampleRay(origin, direction);
}

void Renderer::RetrieveAccumulationResult(
    glm::vec4 *accumulation_color_buffer_dst,
    float *accumulation_number_buffer_dst) {
  std::unique_lock<std::mutex> lock(task_queue_mutex_);
  std::memcpy(accumulation_color_buffer_dst, accumulation_color_.data(),
              sizeof(glm::vec4) * accumulation_color_.size());
  std::memcpy(accumulation_number_buffer_dst, accumulation_number_.data(),
              sizeof(float) * accumulation_number_.size());
}

glm::vec3 Renderer::SampleRay(glm::vec3 origin, glm::vec3 direction) const {
  float x = scene_.GetEnvmapOffset();
  float y = acos(direction.y) * INV_PI;
  if (glm::length(glm::vec2{direction.x, direction.y}) > 1e-4) {
    x += glm::atan(direction.x, -direction.z);
  }
  x *= INV_PI * 0.5;
  return scene_.GetTextures()[scene_.GetEnvmapId()].Sample(glm::vec2{x, y});
}

}  // namespace sparks
