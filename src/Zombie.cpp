#include "Zombie.hpp"

namespace {
glm::vec2 MinCorner(const Util::GameObject &obj) {
  const glm::vec2 size = obj.GetScaledSize();
  return obj.m_Transform.translation - size * 0.5F;
}

glm::vec2 MaxCorner(const Util::GameObject &obj) {
  const glm::vec2 size = obj.GetScaledSize();
  return obj.m_Transform.translation + size * 0.5F;
}
} // namespace

Zombie::Zombie(const std::vector<std::string> &walkingFrames,
               const std::vector<std::string> &attackingFrames,
               const std::vector<std::string> &dyingFrames,
               const float targetHeightPx, const std::size_t frameIntervalMs,
               const float moveSpeedPxPerSec, const int health)
    : m_MoveSpeedPxPerSec(moveSpeedPxPerSec), m_Health(glm::max(1, health)) {
  if (!walkingFrames.empty()) {
    m_WalkingAnimation = std::make_shared<Util::Animation>(
        walkingFrames, true, frameIntervalMs, true, 0);
  }
  if (!attackingFrames.empty()) {
    m_AttackingAnimation = std::make_shared<Util::Animation>(
        attackingFrames, true, frameIntervalMs, true, 0);
  }
  if (!dyingFrames.empty()) {
    m_DyingAnimation = std::make_shared<Util::Animation>(
        dyingFrames, true, frameIntervalMs, false, 0);
  }

  SetZIndex(1.0F);
  EnterState(State::Walking);

  if (targetHeightPx > 0.0F) {
    const float drawableHeight =
        GetScaledSize().y / glm::max(m_Transform.scale.y, 0.0001F);
    if (drawableHeight > 0.0F) {
      const float scale = targetHeightPx / drawableHeight;
      m_Transform.scale = {scale, scale};
    }
  }
}

void Zombie::Update(const float dt,
                    const std::vector<std::shared_ptr<Plant>> &plants) {
  if (m_Destroyed || dt <= 0.0F) {
    return;
  }

  switch (m_State) {
  case State::Walking: {
    m_Transform.translation.x -= m_MoveSpeedPxPerSec * dt;
    m_CurrentTarget = FindCollidingPlant(plants);
    if (m_CurrentTarget != nullptr) {
      EnterState(State::Attacking);
    }
    break;
  }

  case State::Attacking: {
    if (m_CurrentTarget == nullptr || m_CurrentTarget->IsDead()) {
      m_CurrentTarget = FindCollidingPlant(plants);
      if (m_CurrentTarget == nullptr) {
        EnterState(State::Walking);
        break;
      }
    } else {
      // Use distance tolerance instead of AABB to prevent jittering at
      // boundaries
      const float distanceToTarget =
          m_CurrentTarget->m_Transform.translation.x -
          m_Transform.translation.x;
      if (distanceToTarget > m_AttackRangeX) {
        m_CurrentTarget = nullptr;
        EnterState(State::Walking);
        break;
      }
    }

    m_AttackElapsed += dt;
    if (m_AttackElapsed >= m_AttackIntervalSec && m_CurrentTarget != nullptr) {
      m_CurrentTarget->TakeDamage(m_AttackDamage);
      m_AttackElapsed = 0.0F;

      if (m_CurrentTarget->IsDead()) {
        m_CurrentTarget = nullptr;
        EnterState(State::Walking);
      }
    }
    break;
  }

  case State::Dying: {
    if (m_DyingAnimation != nullptr) {
      if (m_DyingAnimation->GetState() == Util::Animation::State::ENDED) {
        m_Destroyed = true;
        SetVisible(false);
      }
      break;
    }

    m_DyingElapsed += dt;
    if (m_DyingElapsed >= m_DyingDurationSec) {
      m_Destroyed = true;
      SetVisible(false);
    }
    break;
  }
  }
}

void Zombie::TakeDamage(const int amount) {
  if (m_State == State::Dying || amount <= 0) {
    return;
  }

  m_Health = glm::max(0, m_Health - amount);
  if (m_Health == 0) {
    EnterState(State::Dying);
  }
}

bool Zombie::CheckAABBCollision(const Util::GameObject &a,
                                const Util::GameObject &b) {
  const glm::vec2 aMin = MinCorner(a);
  const glm::vec2 aMax = MaxCorner(a);
  const glm::vec2 bMin = MinCorner(b);
  const glm::vec2 bMax = MaxCorner(b);

  const bool overlapX = aMin.x <= bMax.x && aMax.x >= bMin.x;
  const bool overlapY = aMin.y <= bMax.y && aMax.y >= bMin.y;
  return overlapX && overlapY;
}

std::shared_ptr<Plant> Zombie::FindCollidingPlant(
    const std::vector<std::shared_ptr<Plant>> &plants) const {
  for (const auto &plant : plants) {
    if (plant == nullptr || plant->IsDead()) {
      continue;
    }

    if (CheckAABBCollision(*this, *plant)) {
      return plant;
    }
  }

  return nullptr;
}

void Zombie::EnterState(const State newState) {
  m_State = newState;

  switch (m_State) {
  case State::Walking:
    m_AttackElapsed = 0.0F;
    if (m_WalkingAnimation != nullptr) {
      SetDrawable(m_WalkingAnimation);
    }
    break;

  case State::Attacking:
    m_AttackElapsed = 0.0F;
    if (m_AttackingAnimation != nullptr) {
      SetDrawable(m_AttackingAnimation);
    }
    break;

  case State::Dying:
    m_DyingElapsed = 0.0F;
    m_CurrentTarget = nullptr;
    if (m_DyingAnimation != nullptr) {
      m_DyingAnimation->SetCurrentFrame(0);
      m_DyingAnimation->Play();
      SetDrawable(m_DyingAnimation);
    }
    break;
  }
}
