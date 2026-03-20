#ifndef PEASHOOTER_HPP
#define PEASHOOTER_HPP

#include "Plant.hpp"
#include "Util/Animation.hpp"

class Peashooter : public Plant {
public:
  // framePaths    - extracted PNG frames from the GIF
  // frameInterval - milliseconds per frame
  // targetHeight  - desired height in pixels (image is scaled proportionally)
  Peashooter(const std::vector<std::string> &framePaths,
             std::size_t frameIntervalMs, float targetHeight);

  bool StartAttack(const std::vector<std::string> &framePaths,
                   std::size_t frameIntervalMs);
  bool UpdateAttackStateAndCheckShoot();
  bool IsAttacking() const { return m_AttackAnimation != nullptr; }

private:
  void ApplyScaleForCurrentDrawable(float targetHeight);

private:
  float m_TargetHeight = 0.0F;
  bool m_HasShotCurrentAttack = false;
  std::shared_ptr<Util::Animation> m_IdleAnimation = nullptr;
  std::shared_ptr<Util::Animation> m_AttackAnimation = nullptr;
};

#endif
