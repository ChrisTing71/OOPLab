#include "Nut.hpp"

namespace {
int StageFromHealth(const int health) {
  if (health <= 1000) {
    return 4;
  }
  if (health <= 2000) {
    return 3;
  }
  if (health <= 3000) {
    return 2;
  }
  return 1;
}
} // namespace

Nut::Nut(const std::vector<std::string> &nut1FramePaths,
         const std::size_t nut1FrameIntervalMs,
         const std::vector<std::string> &nut2FramePaths,
         const std::size_t nut2FrameIntervalMs,
         const std::vector<std::string> &nut3FramePaths,
         const std::size_t nut3FrameIntervalMs,
         const std::vector<std::string> &nut4FramePaths,
         const std::size_t nut4FrameIntervalMs, const float targetHeight)
    : Plant(kMaxHealth), m_TargetHeight(targetHeight) {
  m_Stage1Animation = std::make_shared<Util::Animation>(
      nut1FramePaths, true, nut1FrameIntervalMs, true, 0);
  m_Stage2Animation = std::make_shared<Util::Animation>(
      nut2FramePaths, true, nut2FrameIntervalMs, true, 0);
  m_Stage3Animation = std::make_shared<Util::Animation>(
      nut3FramePaths, true, nut3FrameIntervalMs, true, 0);
  m_Stage4Animation = std::make_shared<Util::Animation>(
      nut4FramePaths, true, nut4FrameIntervalMs, true, 0);

  SetDrawable(m_Stage1Animation);
  SetZIndex(1.0F);
  ApplyScaleForCurrentDrawable(targetHeight);
}

void Nut::TakeDamage(const int amount) {
  const int previousHealth = GetHealth();
  Plant::TakeDamage(amount);
  if (GetHealth() == previousHealth) {
    return;
  }

  UpdateStageDrawableByHealth();
}

void Nut::ApplyScaleForCurrentDrawable(const float targetHeight) {
  const float drawableHeight =
      GetScaledSize().y / glm::max(m_Transform.scale.y, 0.0001F);
  if (drawableHeight > 0.0F) {
    const float uniformScale = targetHeight / drawableHeight;
    m_Transform.scale = {uniformScale, uniformScale};
  }
}

void Nut::UpdateStageDrawableByHealth() {
  const int nextStage = StageFromHealth(GetHealth());
  if (nextStage == m_CurrentStage) {
    return;
  }

  m_CurrentStage = nextStage;
  if (m_CurrentStage == 2) {
    SetDrawable(m_Stage2Animation);
  } else if (m_CurrentStage == 3) {
    SetDrawable(m_Stage3Animation);
  } else if (m_CurrentStage == 4) {
    SetDrawable(m_Stage4Animation);
  } else {
    SetDrawable(m_Stage1Animation);
  }

  ApplyScaleForCurrentDrawable(m_TargetHeight);
}
