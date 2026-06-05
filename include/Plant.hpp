#ifndef PLANT_HPP
#define PLANT_HPP

#include "pch.hpp" // IWYU pragma: export

#include "TimerSystem.hpp"
#include "Util/GameObject.hpp"

class Plant : public Util::GameObject {
public:
  Plant() = default;
  explicit Plant(const int health) : m_Health(glm::max(0, health)) {}
  virtual ~Plant() = default;

  int GetHealth() const { return m_Health; }
  bool IsDead() const { return m_Health <= 0; }
  int GetGridRow() const { return m_GridRow; }
  void SetGridRow(const int row) { m_GridRow = row; }

  // ── Placement cooldown (card cooldown shown in the UI) ──────────────────
  float GetCooldownTime() const { return m_PlacementCooldown.GetTotal(); }
  void SetCooldownTime(float seconds) {
    m_PlacementCooldown.ResetTo(glm::max(0.0F, seconds));
    m_PlacementCooldown.remaining = 0.0F; // don't auto-start
  }
  float GetRemainingCooldown() const {
    return m_PlacementCooldown.GetRemaining();
  }
  bool IsCoolingDown() const { return m_PlacementCooldown.IsRunning(); }
  void UpdateCooldown(float deltaTime) { m_PlacementCooldown.Tick(deltaTime); }
  void StartCooldown() { m_PlacementCooldown.Reset(); }

  // ── Overridable plant behaviour ─────────────────────────────────────────
  virtual bool CanShoot() const { return false; }
  virtual bool ProducesSun() const { return false; }

  virtual void TakeDamage(const int amount) {
    if (amount <= 0 || IsDead()) {
      return;
    }

    m_Health = glm::max(0, m_Health - amount);
    if (m_Health == 0) {
      SetVisible(false);
    }
  }

  virtual void OnProducedSunCollected() {}

private:
  int m_Health = 300;
  int m_GridRow = -1;
  Timer m_PlacementCooldown{7.5F}; // default card cooldown
};

#endif
