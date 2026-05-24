#ifndef POLEVAULTING_ZOMBIE_HPP
#define POLEVAULTING_ZOMBIE_HPP

#include "Zombie.hpp"

class PolevaultingZombie : public Zombie {
public:
  enum class PolevaultState {
    Stand,
    Running,
    Jump1,
    Jump2,
    Walking,
    Attacking,
  };

  PolevaultingZombie(const std::vector<std::string> &standFrames,
                     const std::vector<std::string> &runFrames,
                     const std::vector<std::string> &walkFrames,
                     const std::vector<std::string> &attackingFrames,
                     const std::vector<std::string> &jump1Frames,
                     const std::vector<std::string> &jump2Frames,
                     const std::vector<std::string> &dyingFrames,
                     const std::vector<std::string> &cherryBombDyingFrames,
                     float targetHeightPx, std::size_t frameIntervalMs = 120,
                     float runSpeedPxPerSec = 17.0F,
                     float walkSpeedPxPerSec = 17.0F, int health = 370);

  ~PolevaultingZombie() override = default;

  void Update(float dt,
              const std::vector<std::shared_ptr<Plant>> &plants) override;

  void SetLandingYPosition(float y) { m_LandingYPosition = y; }

private:
  std::shared_ptr<Util::Animation> m_StandAnimation;
  std::shared_ptr<Util::Animation> m_RunAnimation;
  std::shared_ptr<Util::Animation> m_PostJumpWalkAnimation;
  std::shared_ptr<Util::Animation> m_Jump1Animation;
  std::shared_ptr<Util::Animation> m_Jump2Animation;

  PolevaultState m_State = PolevaultState::Stand;
  bool m_HasJumped = false;
  float m_StandElapsed = 0.0F;
  float m_RunSpeedPxPerSec = 17.0F;
  float m_WalkSpeedPxPerSec = 17.0F;
  float m_LandingYPosition = 0.0F;

  std::shared_ptr<Plant> m_CurrentTarget = nullptr;
  float m_AttackElapsed = 0.0F;
  float m_AttackIntervalSec = 1.0F;
  int m_AttackDamage = 100;

  std::shared_ptr<Plant>
  FindCollidingPlant(const std::vector<std::shared_ptr<Plant>> &plants) const;

  void SetAnimation(const std::shared_ptr<Util::Animation> &animation,
                    bool play = true);
  void EnterStand();
  void EnterRunning();
  void EnterJump1();
  void EnterJump2();
  void EnterWalking();
  void EnterAttacking();
};

#endif
