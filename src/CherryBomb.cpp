#include "CherryBomb.hpp"

CherryBomb::CherryBomb(const std::vector<std::string> &idleFramePaths,
                       const std::size_t idleFrameIntervalMs,
                       const std::vector<std::string> &blowFramePaths,
                       const std::size_t blowFrameIntervalMs,
                       const float targetHeightPx, const float explodeDelaySec)
    : m_TargetHeightPx(targetHeightPx), m_ExplodeDelaySec(explodeDelaySec) {
  m_IdleAnimation = std::make_shared<Util::Animation>(
      idleFramePaths, true, idleFrameIntervalMs, true, 0);
  m_BlowAnimation = std::make_shared<Util::Animation>(
      blowFramePaths, true, blowFrameIntervalMs, false, 0);

  SetDrawable(m_IdleAnimation);
  SetZIndex(1.0F);
  ApplyScaleForCurrentDrawable(m_TargetHeightPx);
}

bool CherryBomb::UpdateAndCheckExplode(const float deltaTime) {
  if (IsDead() || m_IsFinished || deltaTime <= 0.0F) {
    return false;
  }

  if (!m_IsExploding) {
    m_ExplodeDelaySec -= deltaTime;
    if (m_ExplodeDelaySec > 0.0F) {
      return false;
    }

    m_IsExploding = true;
    SetDrawable(m_BlowAnimation);
    ApplyScaleForCurrentDrawable(m_TargetHeightPx * 3.0F);
    return true;
  }

  if (m_BlowAnimation != nullptr &&
      m_BlowAnimation->GetState() == Util::Animation::State::ENDED) {
    m_IsFinished = true;
    TakeDamage(GetHealth());
    SetVisible(false);
  }

  return false;
}

void CherryBomb::ApplyScaleForCurrentDrawable(const float targetHeightPx) {
  const float drawableHeight =
      GetScaledSize().y / glm::max(m_Transform.scale.y, 0.0001F);
  if (drawableHeight > 0.0F) {
    const float uniformScale = targetHeightPx / drawableHeight;
    m_Transform.scale = {uniformScale, uniformScale};
  }
}