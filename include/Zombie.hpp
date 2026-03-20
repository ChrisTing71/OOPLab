#ifndef ZOMBIE_HPP
#define ZOMBIE_HPP

#include "pch.hpp" // IWYU pragma: export

#include "Plant.hpp"
#include "Util/Animation.hpp"

class Zombie : public Util::GameObject {
public:
  enum class State {
    Walking,
    Attacking,
    Dying,
  };

  Zombie(const std::vector<std::string> &walkingFrames,
         const std::vector<std::string> &attackingFrames,
         const std::vector<std::string> &dyingFrames,
         std::size_t frameIntervalMs = 120, float moveSpeedPxPerSec = 40.0F,
         int health = 200);

  void Update(float dt, const std::vector<std::shared_ptr<Plant>> &plants);
  void TakeDamage(int amount);

  State GetState() const { return m_State; }
  bool IsDestroyed() const { return m_Destroyed; }

  static bool CheckAABBCollision(const Util::GameObject &a,
                                 const Util::GameObject &b);

private:
  std::shared_ptr<Plant>
  FindCollidingPlant(const std::vector<std::shared_ptr<Plant>> &plants) const;
  void EnterState(State newState);

private:
  State m_State = State::Walking;
  bool m_Destroyed = false;

  float m_MoveSpeedPxPerSec = 40.0F;
  int m_Health = 200;

  int m_AttackDamage = 20;
  float m_AttackIntervalSec = 1.0F;
  float m_AttackElapsed = 0.0F;

  float m_DyingDurationSec = 0.8F;
  float m_DyingElapsed = 0.0F;

  std::shared_ptr<Plant> m_CurrentTarget = nullptr;

  std::shared_ptr<Util::Animation> m_WalkingAnimation = nullptr;
  std::shared_ptr<Util::Animation> m_AttackingAnimation = nullptr;
  std::shared_ptr<Util::Animation> m_DyingAnimation = nullptr;
};

#endif
