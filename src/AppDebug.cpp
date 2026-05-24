#include "App.hpp"

#include "AppDebugConfig.hpp"

#include "CollisionSystem.hpp"
#include "ZombieCollisionBoxHelper.hpp"
#include "config.hpp"

namespace {} // namespace

void App::DebugDrawMouseOverlay() const {
  if constexpr (!AppDebug::kEnableMouseCoordinates) {
    return;
  }

  if (!m_HasClickedPoint) {
    return;
  }

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

void App::DebugDrawCollisionBoxes() const {
  if constexpr (!AppDebug::kEnableCollisionBoxes) {
    return;
  }

  ImDrawList *drawList = ImGui::GetForegroundDrawList();

  for (const auto &zombie : m_ActiveZombies) {
    if (zombie.object == nullptr || zombie.object->IsDestroyed()) {
      continue;
    }

    const bool isPolevaultingZombie =
        dynamic_cast<PolevaultingZombie *>(zombie.object.get()) != nullptr;
    const auto zombieType =
        ZombieCollisionBoxHelper::GetZombieCollisionBoxType(*zombie.object);
    const auto bounds =
        CollisionSystem::GetCollisionBoxBounds(*zombie.object, zombieType);

    const float screenMinX = bounds.minX + m_CameraCurrentX +
                             static_cast<float>(WINDOW_WIDTH) * 0.5F;
    const float screenMaxX = bounds.maxX + m_CameraCurrentX +
                             static_cast<float>(WINDOW_WIDTH) * 0.5F;
    const float screenMinY =
        static_cast<float>(WINDOW_HEIGHT) * 0.5F - bounds.maxY;
    const float screenMaxY =
        static_cast<float>(WINDOW_HEIGHT) * 0.5F - bounds.minY;

    const ImU32 color = isPolevaultingZombie ? IM_COL32(255, 0, 0, 200)
                                             : IM_COL32(0, 255, 0, 200);

    drawList->AddRect(ImVec2(screenMinX, screenMinY),
                      ImVec2(screenMaxX, screenMaxY), color, 0.0F, 0, 2.0F);
  }

  const auto plants = CollectAlivePlants();
  for (const auto &plant : plants) {
    if (plant == nullptr || plant->IsDead()) {
      continue;
    }

    const auto bounds = CollisionSystem::GetCollisionBoxBounds(
        *plant, CollisionSystem::CollisionBoxType::Plant);
    const float screenMinX = bounds.minX + m_CameraCurrentX +
                             static_cast<float>(WINDOW_WIDTH) * 0.5F;
    const float screenMaxX = bounds.maxX + m_CameraCurrentX +
                             static_cast<float>(WINDOW_WIDTH) * 0.5F;
    const float screenMinY =
        static_cast<float>(WINDOW_HEIGHT) * 0.5F - bounds.maxY;
    const float screenMaxY =
        static_cast<float>(WINDOW_HEIGHT) * 0.5F - bounds.minY;

    drawList->AddRect(ImVec2(screenMinX, screenMinY),
                      ImVec2(screenMaxX, screenMaxY), IM_COL32(0, 0, 255, 200),
                      0.0F, 0, 2.0F);
  }

  ImGui::SetNextWindowPos(ImVec2(16.0F, 100.0F), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.70F);
  ImGui::Begin(
      "Zombie Debug", nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
          ImGuiWindowFlags_NoSavedSettings |
          ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

  for (const auto &zombie : m_ActiveZombies) {
    if (zombie.object == nullptr || zombie.object->IsDestroyed()) {
      continue;
    }

    const bool isPolevaultingZombie =
        dynamic_cast<PolevaultingZombie *>(zombie.object.get()) != nullptr;
    const int gridRow = zombie.object->GetGridRow();

    ImGui::Text("%s at row %d (grid row check)",
                isPolevaultingZombie ? "Polevault" : "Normal", gridRow);
  }

  ImGui::Text("Plants:");
  const auto alivePlants = CollectAlivePlants();
  for (const auto &plant : alivePlants) {
    if (plant == nullptr || plant->IsDead()) {
      continue;
    }

    ImGui::Text("Plant at row %d", plant->GetGridRow());
  }

  ImGui::End();
}