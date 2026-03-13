#include "App.hpp"

#include "config.hpp"

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include "Util/Time.hpp"

namespace {
float Lerp(const float from, const float to, const float t) {
  return from + (to - from) * glm::clamp(t, 0.0F, 1.0F);
}
} // namespace

void App::Start() {
  LOG_TRACE("Start");

  m_Map->SetDrawable(std::make_shared<Util::Image>("Resources/map.png"));
  m_Map->SetZIndex(0.0F);
  m_Map->m_Transform.translation = {0.0F, 0.0F};

  const glm::vec2 mapSize = m_Map->GetScaledSize();
  constexpr float kZoom = 1.3F;
  m_Map->m_Transform.scale = {
      (static_cast<float>(WINDOW_WIDTH) / mapSize.x) * kZoom,
      (static_cast<float>(WINDOW_HEIGHT) / mapSize.y) * kZoom,
  };

  m_MapScaledWidth = mapSize.x * m_Map->m_Transform.scale.x;
  m_Root.AddChild(m_Map);

  m_CameraStage = CameraStage::STAGE1_HOME;
  m_CameraStageElapsed = 0.0F;
  m_CameraInitialized = false;

  m_CurrentState = State::UPDATE;
}

void App::UpdateCamera(const float deltaTime) {
  constexpr float kHouseHoldSeconds = 3.0F;
  constexpr float kStage2MoveSeconds = 2.0F;
  constexpr float kRoadHoldSeconds = 3.0F;
  constexpr float kPanToCenterSeconds = 1.5F;

  const float halfVisibleWidth = static_cast<float>(WINDOW_WIDTH) * 0.5F;
  const float halfMapWidth = m_MapScaledWidth * 0.5F;
  const float panLimit = glm::max(0.0F, halfMapWidth - halfVisibleWidth);

  const float leftCameraX = panLimit;
  const float rightCameraX = -panLimit;
  const float centerCameraX =
      glm::clamp(0.10F * m_MapScaledWidth, -panLimit, panLimit);
  constexpr float kCameraOffsetY = 0.05F * static_cast<float>(WINDOW_HEIGHT);

  if (!m_CameraInitialized) {
    m_CameraCurrentX = leftCameraX;
    m_Root.SetTranslation({m_CameraCurrentX, kCameraOffsetY});
    m_CameraInitialized = true;
  }

  m_CameraStageElapsed += deltaTime;

  switch (m_CameraStage) {
  case CameraStage::STAGE1_HOME:
    m_CameraCurrentX = leftCameraX;
    if (m_CameraStageElapsed >= kHouseHoldSeconds) {
      m_CameraStage = CameraStage::STAGE2_RIGHT;
      m_CameraStageElapsed = 0.0F;
      m_CameraFromX = leftCameraX;
      m_CameraToX = rightCameraX;
    }
    break;

  case CameraStage::STAGE2_RIGHT: { // NOLINT
    if (m_CameraStageElapsed < kStage2MoveSeconds) {
      const float t = m_CameraStageElapsed / kStage2MoveSeconds;
      m_CameraCurrentX = Lerp(m_CameraFromX, m_CameraToX, t);
      break;
    }

    m_CameraCurrentX = rightCameraX;
    if (m_CameraStageElapsed >= (kStage2MoveSeconds + kRoadHoldSeconds)) {
      m_CameraCurrentX = rightCameraX;
      m_CameraStage = CameraStage::STAGE3_CENTER;
      m_CameraStageElapsed = 0.0F;
      m_CameraFromX = rightCameraX;
      m_CameraToX = centerCameraX;
    }
    break;
  }

  case CameraStage::STAGE3_CENTER: {
    const float t = m_CameraStageElapsed / kPanToCenterSeconds;
    m_CameraCurrentX = Lerp(m_CameraFromX, m_CameraToX, t);
    if (m_CameraStageElapsed >= kPanToCenterSeconds) {
      m_CameraCurrentX = centerCameraX;
      m_CameraStage = CameraStage::FINISHED;
      m_CameraStageElapsed = 0.0F;
    }
    break;
  }

  case CameraStage::FINISHED:
    m_CameraCurrentX = centerCameraX;
    break;
  }

  m_Root.SetTranslation({m_CameraCurrentX, kCameraOffsetY});
}

void App::Update() {
  UpdateCamera(Util::Time::GetDeltaTimeMs() / 1000.0F);
  m_Root.Update();

  /*
   * Do not touch the code below as they serve the purpose for
   * closing the window.
   */
  if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
    m_CurrentState = State::END;
  }
}

void App::End() { // NOLINT(this method will mutate members in the future)
  LOG_TRACE("End");
}
