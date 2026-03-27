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

  void TakeDamage(const int amount) {
    if (amount <= 0 || IsDead()) {
      return;
    }

    m_Health = glm::max(0, m_Health - amount);
  }

private:
  int m_Health = 300;
};

#endif
