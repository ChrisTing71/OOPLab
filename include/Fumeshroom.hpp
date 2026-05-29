#ifndef FUMESHROOM_HPP
#define FUMESHROOM_HPP

#include "Plant.hpp"
#include "Util/Animation.hpp"

class Fumeshroom : public Plant {
public:
  Fumeshroom(const std::vector<std::string> &idleFramePaths,
             std::size_t frameIntervalMs, float targetHeightPx);

  bool StartAttack(const std::vector<std::string> &attackFramePaths,
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
