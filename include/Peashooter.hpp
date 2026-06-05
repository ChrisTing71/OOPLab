#ifndef PEASHOOTER_HPP
#define PEASHOOTER_HPP

#include "Plant.hpp"
#include "Util/Animation.hpp"

class Peashooter : public Plant {
public:
  // framePaths    - extracted PNG frames from the GIF
  // frameInterval - milliseconds per frame
  // targetHeightPx - externally provided sprite height in pixels
  Peashooter(const std::vector<std::string> &framePaths,
             std::size_t frameIntervalMs, float targetHeightPx);

  bool CanShoot() const override { return true; }

  bool StartAttack(const std::vector<std::string> &framePaths,
                   std::size_t frameIntervalMs);
  bool UpdateAttackStateAndCheckShoot();
  bool IsAttacking() const { return m_AttackAnimation != nullptr; }

private:
  void ApplyScaleForCurrentDrawable(float targetHeightPx);

private:
  float m_TargetHeightPx = 0.0F;
  bool m_HasShotCurrentAttack = false;
  std::shared_ptr<Util::Animation> m_IdleAnimation = nullptr;
  std::shared_ptr<Util::Animation> m_AttackAnimation = nullptr;
};

#endif
