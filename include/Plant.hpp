#ifndef PLANT_HPP
#define PLANT_HPP

#include "pch.hpp" // IWYU pragma: export

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

  // Cooldown management
  float GetCooldownTime() const { return m_CooldownTime; }
  void SetCooldownTime(float seconds) {
    m_CooldownTime = glm::max(0.0F, seconds);
  }
  float GetRemainingCooldown() const { return m_RemainingCooldown; }
  bool IsCoolingDown() const { return m_RemainingCooldown > 0.0F; }
  void UpdateCooldown(float deltaTime) {
    if (m_RemainingCooldown > 0.0F) {
      m_RemainingCooldown -= deltaTime;
      if (m_RemainingCooldown < 0.0F) {
        m_RemainingCooldown = 0.0F;
      }
    }
  }
  void StartCooldown() { m_RemainingCooldown = m_CooldownTime; }

  virtual void TakeDamage(const int amount) {
    if (amount <= 0 || IsDead()) {
      return;
    }

    m_Health = glm::max(0, m_Health - amount);
    if (m_Health == 0) {
      // Immediately hide the object so it doesn't appear alive visually
      // until the main loop removes dead plants.
      SetVisible(false);
    }
  }

  virtual void OnProducedSunCollected() {}

private:
  int m_Health = 300;
  int m_GridRow = -1;
  float m_CooldownTime = 7.5F; // Default cooldown in seconds
  float m_RemainingCooldown = 0.0F;
};

#endif
