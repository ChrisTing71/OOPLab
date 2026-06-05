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
         const std::vector<std::string> &cherryBombDyingFrames,
         float targetHeightPx, std::size_t frameIntervalMs = 120,
         float moveSpeedPxPerSec = 40.0F, int health = 200);

  virtual ~Zombie() = default;
  virtual void Update(float dt,
                      const std::vector<std::shared_ptr<Plant>> &plants);
  void TakeDamage(int amount, bool isCherryBombDamage = false);

  // Subclass capability flags used to avoid dynamic_cast at call sites.
  virtual bool CanJumpOverPlants() const { return false; }
  virtual bool HasExtraArmor() const { return false; }

  State GetState() const { return m_State; }
  bool IsDestroyed() const { return m_Destroyed; }
  int GetGridRow() const { return m_GridRow; }
  void SetGridRow(const int row) { m_GridRow = row; }

  // Set a different height to use when dying (e.g., for conehead)
  void SetDeathTargetHeightPx(float heightPx) {
    m_DeathTargetHeightPx = heightPx;
  }

  void SetCherryBombDeathTargetHeightPx(float heightPx) {
    m_CherryBombDeathTargetHeightPx = heightPx;
  }

private:
  std::shared_ptr<Plant>
  FindCollidingPlant(const std::vector<std::shared_ptr<Plant>> &plants) const;
  void EnterState(State newState);

  static constexpr float m_AttackRangeX =
      100.0F; // Tolerance distance to keep attacking

  State m_State = State::Walking;
  bool m_Destroyed = false;
  bool m_IsCherryBombDeath = false;
  int m_GridRow = -1;

protected:
  float m_TargetHeightPx = 0.0F;
  float m_DeathTargetHeightPx = 0.0F; // 0.0 means use m_TargetHeightPx
  float m_CherryBombDeathTargetHeightPx =
      0.0F; // used only for cherry-bomb deaths

  float m_MoveSpeedPxPerSec = 40.0F;
  int m_Health = 200;

  int m_AttackDamage = 100;
  float m_AttackIntervalSec = 1.0F;
  float m_AttackElapsed = 0.0F;

  float m_DyingDurationSec = 0.8F;
  float m_DyingElapsed = 0.0F;

  std::shared_ptr<Plant> m_CurrentTarget = nullptr;

  std::shared_ptr<Util::Animation> m_WalkingAnimation = nullptr;
  std::shared_ptr<Util::Animation> m_AttackingAnimation = nullptr;
  std::shared_ptr<Util::Animation> m_DyingAnimation = nullptr;
  std::shared_ptr<Util::Animation> m_CherryBombDyingAnimation = nullptr;
};

#endif
