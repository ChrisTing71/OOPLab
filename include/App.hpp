#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export

#include "Util/Renderer.hpp"

class App {
public:
  enum class State {
    START,
    UPDATE,
    END,
  };

  State GetCurrentState() const { return m_CurrentState; }

  void Start();

  void Update();

  void End(); // NOLINT(readability-convert-member-functions-to-static)

  enum class CameraStage {
    STAGE1_HOME,
    STAGE2_RIGHT,
    STAGE3_CENTER,
    FINISHED,
  };

private:
  void ValidTask();
  void UpdateCamera(float deltaTime);

private:
  State m_CurrentState = State::START;
  std::shared_ptr<Util::GameObject> m_Map =
      std::make_shared<Util::GameObject>();
  Util::Renderer m_Root;

  CameraStage m_CameraStage = CameraStage::STAGE1_HOME;
  float m_CameraStageElapsed = 0.0F;
  float m_CameraFromX = 0.0F;
  float m_CameraToX = 0.0F;
  float m_CameraCurrentX = 0.0F;

  float m_MapScaledWidth = 0.0F;
  bool m_CameraInitialized = false;
};

#endif
