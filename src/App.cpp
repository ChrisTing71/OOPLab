#include "App.hpp"

#include <filesystem>
#include <random>

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

  const float cellHeightPixel =
      (cellHeightPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  const float targetHeight = cellHeightPixel * 0.7F;

  auto sunflower = std::make_shared<Sunflower>(
      m_SunflowerFramePaths,
      static_cast<std::size_t>(m_SunflowerFrameIntervalMs), targetHeight);
  sunflower->m_Transform.translation = localPosition;

  const int index = row * kGridColumns + column;
  if (m_Sunflowers[static_cast<std::size_t>(index)] != nullptr) {
    m_Root.RemoveChild(m_Sunflowers[static_cast<std::size_t>(index)]);
  }

  m_Sunflowers[static_cast<std::size_t>(index)] = sunflower;
  m_Root.AddChild(sunflower);
}

void App::SpawnFallingSun() {
  constexpr float kSunHeightPercent = 10.0F;
  constexpr float kSpawnYPercent = 20.0F;
  constexpr float kStopMinYPercent = 52.0F;
  constexpr float kStopMaxYPercent = 78.0F;
  std::uniform_real_distribution<float> xPercentDist(8.0F, 92.0F);
  std::uniform_real_distribution<float> stopYPercentDist(kStopMinYPercent,
                                                         kStopMaxYPercent);

  const float spawnXPercent = xPercentDist(m_Random);
  const float spawnPixelX =
      (spawnXPercent / 100.0F) * static_cast<float>(WINDOW_WIDTH);
  const float spawnPixelY =
      (kSpawnYPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);

  const float localX = spawnPixelX - static_cast<float>(WINDOW_WIDTH) * 0.5F;
  const float localY = static_cast<float>(WINDOW_HEIGHT) * 0.5F - spawnPixelY;
    const float stopPixelY =
      (stopYPercentDist(m_Random) / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
    const float stopLocalY =
      static_cast<float>(WINDOW_HEIGHT) * 0.5F - stopPixelY;

  const float sunHeightPx =
      (kSunHeightPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  auto sun = std::make_shared<Sun>(sunHeightPx);
  sun->m_Transform.translation = {localX, localY};

  ActiveSun activeSun;
  activeSun.object = sun;
  activeSun.falling = true;
  activeSun.stopped = false;
  activeSun.stopLocalY = stopLocalY;
  activeSun.fromSky = true;
  activeSun.expires = true;
  m_UIRoot.AddChild(sun);
  m_Suns.push_back(activeSun);
}

void App::SpawnSunFromSunflower(const std::shared_ptr<Sunflower> &sunflower) {
  constexpr float kSunHeightPercent = 10.0F;
  constexpr float kPopDistancePercent = 7.0F;
  constexpr float kCameraOffsetY = 0.05F * static_cast<float>(WINDOW_HEIGHT);

  if (sunflower == nullptr) {
    return;
  }

  const float sunHeightPx =
      (kSunHeightPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  auto sun = std::make_shared<Sun>(sunHeightPx);

  const glm::vec2 rootToScreenOffset = {m_CameraCurrentX, kCameraOffsetY};
  const glm::vec2 startPosition = sunflower->m_Transform.translation +
                                  sunflower->GetSunSpawnOffset() +
                                  rootToScreenOffset;
  const glm::vec2 targetPosition =
      sunflower->m_Transform.translation +
      sunflower->GetSunPopTargetOffset((kPopDistancePercent / 100.0F) *
                                       static_cast<float>(WINDOW_HEIGHT)) +
      rootToScreenOffset;
  sun->m_Transform.translation = startPosition;

  ActiveSun activeSun;
  activeSun.object = sun;
  activeSun.producer = sunflower;
  activeSun.falling = false;
  activeSun.expires = false;
  activeSun.rising = true;
  activeSun.riseStart = startPosition;
  activeSun.riseTarget = targetPosition;
  m_UIRoot.AddChild(sun);
  m_Suns.push_back(activeSun);
}

glm::vec2 App::CardSlotLocalFromSourceCoord(const float sourceX,
                                            const float sourceY) const {
  const glm::vec2 slotSize = m_CardSlot->GetScaledSize();
  const glm::vec2 sourceSize = m_CardSlot->GetSourceSize();
  if (slotSize.x <= 0.0F || slotSize.y <= 0.0F || sourceSize.x <= 0.0F ||
      sourceSize.y <= 0.0F) {
    return {0.0F, 0.0F};
  }

  const float scaleX = slotSize.x / sourceSize.x;
  const float scaleY = slotSize.y / sourceSize.y;

  const float leftPx = (sourceX - sourceSize.x * 0.5F) * scaleX;
  const float topPx = (sourceY - sourceSize.y * 0.5F) * scaleY;

  return {
      m_CardSlot->m_Transform.translation.x + leftPx,
      m_CardSlot->m_Transform.translation.y - topPx,
  };
}

void App::UpdateSuns(const float deltaTime) {
  m_SunSpawnCountdown -= deltaTime;
  if (m_SunSpawnCountdown <= 0.0F) {
    SpawnFallingSun();
    m_SunSpawnCountdown = 12.0F;
  }

  for (const auto &sunflower : m_Sunflowers) {
    if (sunflower == nullptr || !sunflower->ShouldProduceSun(deltaTime)) {
      continue;
    }

    SpawnSunFromSunflower(sunflower);
  }

  const float dropSpeedPx = 0.05F * static_cast<float>(WINDOW_HEIGHT);
  const glm::vec2 collectTargetLocal = CardSlotLocalFromSourceCoord(
      (17.0F + 92.0F) * 0.5F, (15.0F + 91.0F) * 0.5F);
  constexpr float kCollectMoveSeconds = 0.30F;
  constexpr float kRiseMoveSeconds = 0.35F;

  for (std::size_t i = 0; i < m_Suns.size(); ++i) {
    auto &sun = m_Suns[i];

    if (sun.collecting) {
      sun.collectElapsed += deltaTime;
      const float t =
          glm::clamp(sun.collectElapsed / kCollectMoveSeconds, 0.0F, 1.0F);
      sun.object->m_Transform.translation = {
          Lerp(sun.collectStart.x, collectTargetLocal.x, t),
          Lerp(sun.collectStart.y, collectTargetLocal.y, t),
      };
      continue;
    }

    sun.aliveSeconds += deltaTime;

    if (sun.rising) {
      sun.riseElapsed += deltaTime;
      const float t =
          glm::clamp(sun.riseElapsed / kRiseMoveSeconds, 0.0F, 1.0F);
      const float easedT = 1.0F - (1.0F - t) * (1.0F - t);
      sun.object->m_Transform.translation = {
          Lerp(sun.riseStart.x, sun.riseTarget.x, easedT),
          Lerp(sun.riseStart.y, sun.riseTarget.y, easedT),
      };
      if (t >= 1.0F) {
        sun.rising = false;
      }
      continue;
    }

    if (sun.falling) {
      sun.object->m_Transform.translation.y -= dropSpeedPx * deltaTime;

      if (sun.fromSky && !sun.stopped &&
          sun.object->m_Transform.translation.y <= sun.stopLocalY) {
        sun.object->m_Transform.translation.y = sun.stopLocalY;
        sun.falling = false;
        sun.stopped = true;
        sun.stoppedSeconds = 0.0F;
      }
    }

    if (sun.fromSky && sun.stopped) {
      sun.stoppedSeconds += deltaTime;
    }
  }

  for (std::size_t i = 0; i < m_Suns.size();) {
    const auto &sun = m_Suns[i];

    if (sun.collecting && sun.collectElapsed >= kCollectMoveSeconds) {
      if (const auto producer = sun.producer.lock(); producer != nullptr) {
        producer->OnProducedSunCollected();
      }
      RemoveSunAt(i);
      m_Sunlight += 25;
      continue;
    }

    const glm::vec2 sunSize = sun.object->GetScaledSize();
    const float centerY = static_cast<float>(WINDOW_HEIGHT) * 0.5F -
                          sun.object->m_Transform.translation.y;
    const float sunTopPixel = centerY - sunSize.y * 0.5F;
    if (!sun.collecting && sun.expires &&
      ((sun.fromSky && sun.stopped && sun.stoppedSeconds > 5.0F) ||
       (sun.falling && sunTopPixel > static_cast<float>(WINDOW_HEIGHT)))) {
      RemoveSunAt(i);
      continue;
    }

    ++i;
  }
}

bool App::TryCollectSunAt(const float pixelX, const float pixelY) {
  for (std::size_t i = 0; i < m_Suns.size(); ++i) {
    auto &sun = m_Suns[i];
    const glm::vec2 center = {
        sun.object->m_Transform.translation.x +
            static_cast<float>(WINDOW_WIDTH) * 0.5F,
        static_cast<float>(WINDOW_HEIGHT) * 0.5F -
            sun.object->m_Transform.translation.y,
    };
    const glm::vec2 size = sun.object->GetScaledSize();
    const float halfWidth = size.x * 0.5F;
    const float halfHeight = size.y * 0.5F;

    const bool hit =
        (pixelX >= center.x - halfWidth) && (pixelX <= center.x + halfWidth) &&
        (pixelY >= center.y - halfHeight) && (pixelY <= center.y + halfHeight);
    if (!hit) {
      continue;
    }

    if (sun.collecting) {
      return true;
    }

    sun.collecting = true;
    sun.collectElapsed = 0.0F;
    sun.collectStart = sun.object->m_Transform.translation;
    sun.rising = false;
    sun.falling = false;
    return true;
  }

  return false;
}

void App::RemoveSunAt(const std::size_t index) {
  m_UIRoot.RemoveChild(m_Suns[index].object);
  m_Suns.erase(m_Suns.begin() + static_cast<long>(index));
}

void App::DrawSunlightCounter() const {
  if (m_CameraStage != CameraStage::FINISHED) {
    return;
  }

  const glm::vec2 slotSize = m_CardSlot->GetScaledSize();
  const glm::vec2 sourceSize = m_CardSlot->GetSourceSize();
  if (slotSize.x <= 0.0F || slotSize.y <= 0.0F || sourceSize.x <= 0.0F ||
      sourceSize.y <= 0.0F) {
    return;
  }

  const float centerX = m_CardSlot->m_Transform.translation.x +
                        static_cast<float>(WINDOW_WIDTH) * 0.5F;
  const float centerY = static_cast<float>(WINDOW_HEIGHT) * 0.5F -
                        m_CardSlot->m_Transform.translation.y;
  const glm::vec2 topLeft = {centerX - slotSize.x * 0.5F,
                             centerY - slotSize.y * 0.5F};

  const float scaleX = slotSize.x / sourceSize.x;
  const float scaleY = slotSize.y / sourceSize.y;

  constexpr float kCountX1 = 24.0F;
  constexpr float kCountY1 = 102.0F;
  constexpr float kCountX2 = 83.0F;
  constexpr float kCountY2 = 125.0F;

  const ImVec2 rectMin(topLeft.x + kCountX1 * scaleX,
                       topLeft.y + kCountY1 * scaleY);
  const ImVec2 rectMax(topLeft.x + kCountX2 * scaleX,
                       topLeft.y + kCountY2 * scaleY);

  const std::string sunlightText = std::to_string(m_Sunlight);
  const ImVec2 textSize = ImGui::CalcTextSize(sunlightText.c_str());
  const ImVec2 textPos(rectMin.x + (rectMax.x - rectMin.x - textSize.x) * 0.5F,
                       rectMin.y + (rectMax.y - rectMin.y - textSize.y) * 0.5F);

  ImDrawList *drawList = ImGui::GetForegroundDrawList();
  drawList->AddText(textPos, IM_COL32(28, 28, 20, 255), sunlightText.c_str());
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

  // Card slot UI - fixed to screen, shown only after camera settles
  {
    m_CardSlot->SetVisible(false);

    constexpr float kTopPercent = 0.9F;
    constexpr float kBottomPercent = 14.0F;
    const float targetHeightPx = ((kBottomPercent - kTopPercent) / 100.0F) *
                                 static_cast<float>(WINDOW_HEIGHT);
    const glm::vec2 naturalSize = m_CardSlot->GetSourceSize();
    if (naturalSize.y > 0.0F) {
      const float slotScale = targetHeightPx / naturalSize.y;
      m_CardSlot->m_Transform.scale = {slotScale, slotScale};
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

      m_CardSlot->m_Transform.translation = {centerPtsdX, centerPtsdY};
    }
    m_UIRoot.AddChild(m_CardSlot);
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
      m_SunSystemStarted = true;
      m_SunSpawnCountdown = 8.0F;
    }
    break;
  }

  case CameraStage::FINISHED:
    m_CameraCurrentX = centerCameraX;
    m_CardSlot->SetVisible(true);
    break;
  }

  m_Root.SetTranslation({m_CameraCurrentX, kCameraOffsetY});
}

void App::Update() {
  const float deltaTime = Util::Time::GetDeltaTimeMs() / 1000.0F;
  UpdateCamera(deltaTime);
  if (m_SunSystemStarted) {
    UpdateSuns(deltaTime);
  }

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

    const bool collectedSun = TryCollectSunAt(pixelX, pixelY);

    const bool insideGrid =
        (xPercent >= kGridMinXPercent && xPercent <= kGridMaxXPercent) &&
        (yPercent >= kGridMinYPercent && yPercent <= kGridMaxYPercent);

    if (!collectedSun && insideGrid) {
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
  DrawSunlightCounter();

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
