#ifndef BULLET_HPP
#define BULLET_HPP

#include "pch.hpp"

#include "TimerSystem.hpp"
#include "Util/Animation.hpp"
#include "Util/GameObject.hpp"

// Base projectile class for all directed bullets (pea, shroom spore, etc.).
// Inherits from Util::GameObject so it can be added directly to the renderer.
// Movement and out-of-bounds checks remain in App; hit state is self-managed.
class Bullet : public Util::GameObject {
public:
  static constexpr int kDamage = 20;

  // @param row       Grid row the shooter is on (used for same-row collision).
  // @param targetHeightPx  Desired sprite height in pixels for scale fitting.
  Bullet(int row, float targetHeightPx);

  // Transition to hit state. Centers the bullet on @hitCenter and swaps the
  // drawable to the provided hit animation.
  void StartHit(const glm::vec2 &hitCenter,
                const std::vector<std::string> &hitFrames,
                std::size_t hitFrameIntervalMs);

  // Advance hit timer. Returns true when this bullet should be removed.
  bool TickHit(float dt);

  bool IsHitting() const { return m_Hitting; }
  bool IsExpired() const { return m_Expired; }
  int GetRow() const { return m_Row; }

protected:
  float m_TargetHeightPx = 0.0F;

private:
  // Hit display lasts until animation ends OR this many seconds elapse.
  static constexpr float kHitLifetimeSec = 0.2F;

  int m_Row = -1;
  bool m_Hitting = false;
  bool m_Expired = false;
  Timer m_HitTimer{kHitLifetimeSec};
  std::shared_ptr<Util::Animation> m_HitAnimation;
};

#endif // BULLET_HPP
