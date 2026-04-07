#include "Sunflower.hpp"

#include "Util/Animation.hpp"

Sunflower::Sunflower(const std::vector<std::string> &framePaths,
                     const std::size_t frameIntervalMs,
                     const float targetHeightPx) {
  auto animation = std::make_shared<Util::Animation>(framePaths, true,
                                                     frameIntervalMs, true, 0);

  SetDrawable(animation);
  SetZIndex(1.0F);

  const float drawableHeight = animation->GetSize().y;
  if (drawableHeight > 0.0F) {
    const float uniformScale = targetHeightPx / drawableHeight;
    m_Transform.scale = {uniformScale, uniformScale};
  }
}

bool Sunflower::ShouldProduceSun(const float deltaTime) {
  if (m_HasActiveProducedSun) {
    return false;
  }

  m_TimeUntilNextSun -= deltaTime;
  if (m_TimeUntilNextSun > 0.0F) {
    return false;
  }

  m_HasActiveProducedSun = true;
  return true;
}

void Sunflower::OnProducedSunCollected() {
  m_HasActiveProducedSun = false;
  m_TimeUntilNextSun = 24.0F;
}

glm::vec2 Sunflower::GetSunSpawnOffset() const {
  const glm::vec2 size = GetScaledSize();
  return {0.0F, size.y * -0.2F};
}

glm::vec2 Sunflower::GetSunPopTargetOffset(const float popDistancePx) const {
  const glm::vec2 start = GetSunSpawnOffset();
  return {start.x, start.y + popDistancePx};
}
