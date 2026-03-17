#include "App.hpp"

#include <filesystem>

#include "config.hpp"

#include "Util/Animation.hpp"
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

bool App::PrepareSunflowerFrames() {
  if (!m_SunflowerFramePaths.empty()) {
    return true;
  }

  constexpr std::string_view kSunflowerFramesDir = "Resources/sunflower_frames";
  std::error_code mkdirError;
  std::filesystem::create_directories(kSunflowerFramesDir, mkdirError);
  if (mkdirError) {
    LOG_WARN("Failed to create sunflower frames folder: {}",
             mkdirError.message());
  }

  IMG_Animation *gif = IMG_LoadAnimation("Resources/sunflower.gif");
  if (gif == nullptr || gif->count <= 0) {
    LOG_ERROR("Failed to load sunflower.gif animation: {}", IMG_GetError());
    if (gif != nullptr) {
      IMG_FreeAnimation(gif);
    }
    return false;
  }

  m_SunflowerFramePaths.reserve(static_cast<std::size_t>(gif->count));
  int delaySum = 0;
  int delayCount = 0;

  for (int i = 0; i < gif->count; ++i) {
    const std::string framePath =
        fmt::format("{}/sunflower_frame_{:03d}.png", kSunflowerFramesDir, i);
    if (IMG_SavePNG(gif->frames[i], framePath.c_str()) != 0) {
      LOG_WARN("Failed to save sunflower frame {}: {}", i, IMG_GetError());
      continue;
    }

    m_SunflowerFramePaths.push_back(framePath);

    if (gif->delays != nullptr && gif->delays[i] > 0) {
      delaySum += gif->delays[i];
      ++delayCount;
    }
  }

  if (delayCount > 0) {
    m_SunflowerFrameIntervalMs = glm::max(30, delaySum / delayCount);
  }

  IMG_FreeAnimation(gif);
  return !m_SunflowerFramePaths.empty();
}

void App::PlaceSunflowerAtGridCell(const int row, const int column) {
  if (!PrepareSunflowerFrames()) {
    return;
  }

  const float cellWidthPercent =
      (kGridMaxXPercent - kGridMinXPercent) / static_cast<float>(kGridColumns);
  const float cellHeightPercent =
      (kGridMaxYPercent - kGridMinYPercent) / static_cast<float>(kGridRows);

  const float centerXPercent =
      kGridMinXPercent + (static_cast<float>(column) + 0.5F) * cellWidthPercent;
  const float centerYPercent =
      kGridMinYPercent + (static_cast<float>(row) + 0.5F) * cellHeightPercent;

  const float centerPixelX =
      (centerXPercent / 100.0F) * static_cast<float>(WINDOW_WIDTH);
  const float centerPixelY =
      (centerYPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);

  const float centerCursorX =
      centerPixelX - static_cast<float>(WINDOW_WIDTH) * 0.5F;
  const float centerCursorY =
      static_cast<float>(WINDOW_HEIGHT) * 0.5F - centerPixelY;

  constexpr float kCameraOffsetY = 0.05F * static_cast<float>(WINDOW_HEIGHT);
  const glm::vec2 localPosition = glm::vec2(centerCursorX, centerCursorY) -
                                  glm::vec2(m_CameraCurrentX, kCameraOffsetY);

  auto sunflower = std::make_shared<Util::GameObject>();
  auto sunflowerAnimation = std::make_shared<Util::Animation>(
      m_SunflowerFramePaths, true,
      static_cast<std::size_t>(m_SunflowerFrameIntervalMs), true, 0);

  sunflower->SetDrawable(sunflowerAnimation);
  sunflower->SetZIndex(1.0F);
  sunflower->m_Transform.translation = localPosition;

  const float cellHeightPixel =
      (cellHeightPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  const float targetHeight = cellHeightPixel * 0.7F;
  const float drawableHeight = sunflowerAnimation->GetSize().y;
  if (drawableHeight > 0.0F) {
    const float uniformScale = targetHeight / drawableHeight;
    sunflower->m_Transform.scale = {uniformScale, uniformScale};
  }

  const int index = row * kGridColumns + column;
  if (m_Sunflowers[static_cast<std::size_t>(index)] != nullptr) {
    m_Root.RemoveChild(m_Sunflowers[static_cast<std::size_t>(index)]);
  }

  m_Sunflowers[static_cast<std::size_t>(index)] = sunflower;
  m_Root.AddChild(sunflower);
}

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

  // Upper slots UI – fixed to screen, shown only after camera settles
  {
    auto slotImg = std::make_shared<Util::Image>("Resources/UpperSlot.png");
    m_UpperSlots->SetDrawable(slotImg);
    m_UpperSlots->SetZIndex(10.0F);
    m_UpperSlots->SetVisible(false);

    constexpr float kTopPercent = 0.9F;
    constexpr float kBottomPercent = 14.0F;
    const float targetHeightPx = ((kBottomPercent - kTopPercent) / 100.0F) *
                                 static_cast<float>(WINDOW_HEIGHT);
    const glm::vec2 naturalSize = slotImg->GetSize();
    if (naturalSize.y > 0.0F) {
      const float slotScale = targetHeightPx / naturalSize.y;
      m_UpperSlots->m_Transform.scale = {slotScale, slotScale};
      const float scaledWidth = naturalSize.x * slotScale;

      // Left edge at x = 21 %
      const float leftEdgePx = 0.21F * static_cast<float>(WINDOW_WIDTH);
      const float centerPtsdX = leftEdgePx + scaledWidth * 0.5F -
                                static_cast<float>(WINDOW_WIDTH) * 0.5F;

      // Vertical centre between 0.9 % and 12 % from top
      const float topPx =
          (kTopPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
      const float bottomPx =
          (kBottomPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
      const float centerPtsdY =
          static_cast<float>(WINDOW_HEIGHT) * 0.5F - (topPx + bottomPx) * 0.5F;

      m_UpperSlots->m_Transform.translation = {centerPtsdX, centerPtsdY};
    }
    m_UIRoot.AddChild(m_UpperSlots);
  }

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
    m_UpperSlots->SetVisible(true);
    break;
  }

  m_Root.SetTranslation({m_CameraCurrentX, kCameraOffsetY});
}

void App::Update() {
  UpdateCamera(Util::Time::GetDeltaTimeMs() / 1000.0F);

  if (Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB)) {
    const glm::vec2 cursor = Util::Input::GetCursorPosition();
    const float pixelX = cursor.x + static_cast<float>(WINDOW_WIDTH) * 0.5F;
    const float pixelY = static_cast<float>(WINDOW_HEIGHT) * 0.5F - cursor.y;

    const float xPercent = (pixelX / static_cast<float>(WINDOW_WIDTH)) * 100.0F;
    const float yPercent =
        (pixelY / static_cast<float>(WINDOW_HEIGHT)) * 100.0F;

    m_HasClickedPoint = true;
    m_LastClickPixel = {pixelX, pixelY};
    m_LastClickPercent = {glm::clamp(xPercent, 0.0F, 100.0F),
                          glm::clamp(yPercent, 0.0F, 100.0F)};

    const bool insideGrid =
        (xPercent >= kGridMinXPercent && xPercent <= kGridMaxXPercent) &&
        (yPercent >= kGridMinYPercent && yPercent <= kGridMaxYPercent);

    if (insideGrid) {
      const float normalizedX =
          (xPercent - kGridMinXPercent) / (kGridMaxXPercent - kGridMinXPercent);
      const float normalizedY =
          (yPercent - kGridMinYPercent) / (kGridMaxYPercent - kGridMinYPercent);

      const int column = glm::clamp(
          static_cast<int>(normalizedX * kGridColumns), 0, kGridColumns - 1);
      const int row = glm::clamp(static_cast<int>(normalizedY * kGridRows), 0,
                                 kGridRows - 1);

      m_LastHitColumn = column + 1;
      m_LastHitRow = row + 1;
      m_HasGridHit = true;

      PlaceSunflowerAtGridCell(row, column);
    } else {
      m_HasGridHit = false;
    }
  }

  m_Root.Update();
  m_UIRoot.Update();

  if (m_HasClickedPoint) {
    ImGui::SetNextWindowPos(ImVec2(16.0F, 16.0F), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.70F);
    ImGui::Begin(
        "Click Debug", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::Text("pixel: (%.1f, %.1f)", m_LastClickPixel.x, m_LastClickPixel.y);
    ImGui::Text("percent: x=%.2f%%, y=%.2f%%", m_LastClickPercent.x,
                m_LastClickPercent.y);
    ImGui::End();

    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    const ImVec2 clickPos(m_LastClickPixel.x, m_LastClickPixel.y);
    drawList->AddCircleFilled(clickPos, 4.0F, IM_COL32(255, 70, 70, 255));
    drawList->AddLine(ImVec2(clickPos.x - 10.0F, clickPos.y),
                      ImVec2(clickPos.x + 10.0F, clickPos.y),
                      IM_COL32(255, 70, 70, 255), 2.0F);
    drawList->AddLine(ImVec2(clickPos.x, clickPos.y - 10.0F),
                      ImVec2(clickPos.x, clickPos.y + 10.0F),
                      IM_COL32(255, 70, 70, 255), 2.0F);
  }

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
