#pragma once
#include "condition_variable"
#include "mutex"
#include "queue"
#include "sparks/assets/assets.h"
#include "sparks/renderer/renderer_settings.h"
#include "sparks/renderer/util.h"
#include "sparks/util/util.h"
#include "thread"

namespace sparks {
class Renderer {
 public:
  explicit Renderer(const RendererSettings &renderer_settings);
  Scene &GetScene();
  [[nodiscard]] const Scene &GetScene() const;
  RendererSettings &GetRendererSettings();
  [[nodiscard]] const RendererSettings &GetRendererSettings() const;

  void StartWorkerThreads();
  void PauseWorkers();
  void ResumeWorkers();
  void StopWorkers();

  [[nodiscard]] RenderStateSignal GetRenderStateSignal() const;
  void Resize(uint32_t width, uint32_t height);
  void ResetAccumulation();

  void RayGeneration(int x, int y, glm::vec3 &color_result) const;
  [[nodiscard]] glm::vec3 SampleRay(glm::vec3 origin,
                                    glm::vec3 direction) const;

  void RetrieveAccumulationResult(glm::vec4 *accumulation_color_buffer_dst,
                                  float *accumulation_number_buffer_dst);

 private:
  void WorkerThread();
  RendererSettings renderer_settings_;
  Scene scene_;

  /* CPU Renderer Assets */
  std::vector<glm::vec4> accumulation_color_;
  std::vector<float> accumulation_number_;
  std::queue<TaskInfo> task_queue_;

  std::condition_variable wait_for_queue_cv_;
  std::condition_variable wait_for_resume_cv_;
  std::condition_variable wait_for_all_pause_;
  std::condition_variable wait_for_all_exit_;
  std::mutex task_queue_mutex_;

  std::vector<std::thread> worker_threads_;
  RenderStateSignal render_state_signal_{RENDER_STATE_SIGNAL_RUN};
  uint32_t num_paused_thread_{0};
  uint32_t num_exited_thread_{0};

  uint32_t width_{0};
  uint32_t height_{0};
};
}  // namespace sparks
