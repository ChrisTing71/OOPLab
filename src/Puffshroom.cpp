#include "Puffshroom.hpp"

Puffshroom::Puffshroom(const std::vector<std::string> &framePaths,
                       const std::size_t frameIntervalMs,
                       const float targetHeightPx) {
  // Puffshroom is a short-range shooter; set reasonable internal cooldown
  SetCooldownTime(1.0F);

  m_TargetHeightPx = targetHeightPx;
  if (!framePaths.empty()) {
    m_IdleAnimation = std::make_shared<Util::Animation>(
        framePaths, true, frameIntervalMs, true, 0);
    SetDrawable(m_IdleAnimation);
    SetZIndex(1.0F);
    ApplyScaleForCurrentDrawable(targetHeightPx);
  }
}

void Puffshroom::ApplyScaleForCurrentDrawable(const float targetHeightPx) {
  const float drawableHeight =
      GetScaledSize().y / glm::max(m_Transform.scale.y, 0.0001F);
  if (drawableHeight > 0.0F) {
    const float uniformScale = targetHeightPx / drawableHeight;
    m_Transform.scale = {uniformScale, uniformScale};
  }
}
