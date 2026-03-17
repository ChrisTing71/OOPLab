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
  bool PrepareSunflowerFrames();
  void PlaceSunflowerAtGridCell(int row, int column);

private:
  static constexpr float kGridMinXPercent = 21.0F;
  static constexpr float kGridMaxXPercent = 89.0F;
  static constexpr float kGridMinYPercent = 14.0F;
  static constexpr float kGridMaxYPercent = 98.0F;
  static constexpr int kGridColumns = 9;
  static constexpr int kGridRows = 5;
  static constexpr int kGridCellCount = kGridColumns * kGridRows;

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

  bool m_HasClickedPoint = false;
  bool m_HasGridHit = false;
  glm::vec2 m_LastClickPixel = {0.0F, 0.0F};
  glm::vec2 m_LastClickPercent = {0.0F, 0.0F};
  int m_LastHitRow = 0;
  int m_LastHitColumn = 0;

  std::vector<std::string> m_SunflowerFramePaths;
  int m_SunflowerFrameIntervalMs = 100;
  std::array<std::shared_ptr<Util::GameObject>, kGridCellCount> m_Sunflowers{};

  std::shared_ptr<Util::GameObject> m_UpperSlots =
      std::make_shared<Util::GameObject>();
  Util::Renderer m_UIRoot;
};

#endif
