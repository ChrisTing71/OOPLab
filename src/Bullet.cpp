#include "Bullet.hpp"

Bullet::Bullet(int row, float targetHeightPx)
    : m_TargetHeightPx(targetHeightPx), m_Row(row) {}

void Bullet::StartHit(const glm::vec2 &hitCenter,
                      const std::vector<std::string> &hitFrames,
                      std::size_t hitFrameIntervalMs) {
  m_Hitting = true;
  m_HitTimer.Reset();
  m_Transform.translation = hitCenter;

  m_HitAnimation =
      std::make_shared<Util::Animation>(hitFrames, true, hitFrameIntervalMs,
                                        false, 0);

  // Scale hit animation to match the bullet's current rendered size.
  const glm::vec2 bulletSize = GetScaledSize();
  const glm::vec2 hitSize = m_HitAnimation->GetSize();
  if (hitSize.y > 0.0F && bulletSize.y > 0.0F) {
    const float scale = bulletSize.y / hitSize.y;
    m_Transform.scale = {scale, scale};
  }

  SetDrawable(m_HitAnimation);
}

bool Bullet::TickHit(float dt) {
  if (!m_Hitting) {
    return false;
  }

  m_HitTimer.Tick(dt);

  const bool animEnded = m_HitAnimation != nullptr &&
                         m_HitAnimation->GetState() ==
                             Util::Animation::State::ENDED;

  m_Expired = m_HitTimer.IsExpired() || animEnded;
  return m_Expired;
}
