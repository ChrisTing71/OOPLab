#include "Sunshroom.hpp"

Sunshroom::Sunshroom(const std::vector<std::string> &initialFramePaths,
                     const std::vector<std::string> &grownFramePaths,
                     const std::size_t frameIntervalMs,
                     const float targetHeightPx) {
  if (!initialFramePaths.empty()) {
    m_InitialAnimation = std::make_shared<Util::Animation>(
        initialFramePaths, true, frameIntervalMs, true, 0);
  }
  if (!grownFramePaths.empty()) {
    m_GrownAnimation = std::make_shared<Util::Animation>(
        grownFramePaths, true, frameIntervalMs, true, 0);
  }

  SetCurrentAnimation();
  SetZIndex(1.0F);
  ApplyScaleToDrawable(targetHeightPx);
}

void Sunshroom::Update(float deltaTime) {
  if (deltaTime <= 0.0F) {
    return;
  }

  m_GrowthTime += deltaTime;
  if (m_State == State::Initial && m_GrowthTime >= kGrowUpTimeSeconds) {
    m_State = State::Grown;
    SetCurrentAnimation();
  }
}

bool Sunshroom::ShouldProduceSun(float deltaTime) {
  if (m_HasActiveProducedSun) {
    return false;
  }

  m_TimeUntilNextSun -= deltaTime;
  if (m_TimeUntilNextSun > 0.0F) {
    return false;
  }

  m_ProducedSunValue = m_GrowthTime >= kGrowUpTimeSeconds ? kGrownSunValue : kInitialSunValue;
  m_HasActiveProducedSun = true;
  return true;
}

int Sunshroom::GetProducedSunValue() const {
  return m_ProducedSunValue;
}

int Sunshroom::GetSunValue() const {
  return m_GrowthTime >= kGrowUpTimeSeconds ? kGrownSunValue : kInitialSunValue;
}

int Sunshroom::GetSunValueAtGrowthTime(const float growthTime) const {
  return growthTime >= kGrowUpTimeSeconds ? kGrownSunValue : kInitialSunValue;
}

float Sunshroom::GetGrowthTimeSeconds() const {
  return m_GrowthTime;
}

glm::vec2 Sunshroom::GetSunSpawnOffset() const {
  const glm::vec2 size = GetScaledSize();
  return glm::vec2(0.0F, size.y * -0.2F);
}

glm::vec2 Sunshroom::GetSunPopTargetOffset(float popDistancePx) const {
  const glm::vec2 start = GetSunSpawnOffset();
  return glm::vec2(start.x, start.y + popDistancePx);
}

void Sunshroom::OnProducedSunCollected() {
  m_HasActiveProducedSun = false;
  m_TimeUntilNextSun = 24.0F;
}

void Sunshroom::SetCurrentAnimation() {
  if (m_State == State::Grown && m_GrownAnimation != nullptr) {
    SetDrawable(m_GrownAnimation);
    return;
  }

  if (m_InitialAnimation != nullptr) {
    SetDrawable(m_InitialAnimation);
  }
}

void Sunshroom::ApplyScaleToDrawable(float targetHeightPx) {
  if (m_Drawable == nullptr) {
    return;
  }

  const glm::vec2 imageSize = m_Drawable->GetSize();
  if (imageSize.y <= 0.0F) {
    return;
  }

  const float uniformScale = (targetHeightPx / imageSize.y) * 0.75F;
  m_Transform.scale = {uniformScale, uniformScale};
}
