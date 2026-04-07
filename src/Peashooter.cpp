#include "Peashooter.hpp"

#include "Util/Animation.hpp"

Peashooter::Peashooter(const std::vector<std::string> &framePaths,
                       const std::size_t frameIntervalMs,
                       const float targetHeightPx) {
  m_TargetHeightPx = targetHeightPx;
  m_IdleAnimation = std::make_shared<Util::Animation>(framePaths, true,
                                                      frameIntervalMs, true, 0);

  SetDrawable(m_IdleAnimation);
  SetZIndex(1.0F);
  ApplyScaleForCurrentDrawable(targetHeightPx);
}

bool Peashooter::StartAttack(const std::vector<std::string> &framePaths,
                             const std::size_t frameIntervalMs) {
  if (framePaths.empty() || IsAttacking()) {
    return false;
  }

  m_AttackAnimation = std::make_shared<Util::Animation>(
      framePaths, true, frameIntervalMs, false, 0);
  m_HasShotCurrentAttack = false;
  SetDrawable(m_AttackAnimation);
  ApplyScaleForCurrentDrawable(m_TargetHeightPx);
  return true;
}

bool Peashooter::UpdateAttackStateAndCheckShoot() {
  if (!IsAttacking()) {
    return false;
  }

  constexpr std::size_t kShootFrameIndex = 16; // 17th frame
  bool shouldShoot = false;
  if (!m_HasShotCurrentAttack &&
      m_AttackAnimation->GetCurrentFrameIndex() >= kShootFrameIndex) {
    m_HasShotCurrentAttack = true;
    shouldShoot = true;
  }

  if (m_AttackAnimation->GetState() == Util::Animation::State::ENDED) {
    m_AttackAnimation = nullptr;
    m_HasShotCurrentAttack = false;
    SetDrawable(m_IdleAnimation);
    ApplyScaleForCurrentDrawable(m_TargetHeightPx);
  }

  return shouldShoot;
}

void Peashooter::ApplyScaleForCurrentDrawable(const float targetHeightPx) {
  const float drawableHeight =
      GetScaledSize().y / glm::max(m_Transform.scale.y, 0.0001F);
  if (drawableHeight > 0.0F) {
    const float uniformScale = targetHeightPx / drawableHeight;
    m_Transform.scale = {uniformScale, uniformScale};
  }
}
