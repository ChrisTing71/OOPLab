#include "Zombie.hpp"
#include "PolevaultingZombie.hpp"

#include <limits>

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
               const std::vector<std::string> &cherryBombDyingFrames,
               const float targetHeightPx, const std::size_t frameIntervalMs,
               const float moveSpeedPxPerSec, const int health)
    : m_TargetHeightPx(targetHeightPx), m_MoveSpeedPxPerSec(moveSpeedPxPerSec),
      m_Health(glm::max(1, health)) {
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
  if (!cherryBombDyingFrames.empty()) {
    m_CherryBombDyingAnimation = std::make_shared<Util::Animation>(
        cherryBombDyingFrames, true, frameIntervalMs, false, 0);
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
    const std::shared_ptr<Util::Animation> dyingAnimation =
        m_IsCherryBombDeath ? m_CherryBombDyingAnimation : m_DyingAnimation;

    if (dyingAnimation != nullptr) {
      if (dyingAnimation->GetState() == Util::Animation::State::ENDED) {
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

void Zombie::TakeDamage(const int amount, const bool isCherryBombDamage) {
  if (m_State == State::Dying || amount <= 0) {
    return;
  }

  m_IsCherryBombDeath = isCherryBombDamage;
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
  std::shared_ptr<Plant> bestTarget = nullptr;
  float bestDistance = std::numeric_limits<float>::max();
  const float zombieX = m_Transform.translation.x;

  for (const auto &plant : plants) {
    if (plant == nullptr || plant->IsDead()) {
      continue;
    }

    // Only consider plants that are in front of the zombie's movement
    // direction. Zombies move left, so ignore plants that are behind.
    if (plant->m_Transform.translation.x > zombieX) {
      continue;
    }

    if (!CheckAABBCollision(*this, *plant)) {
      continue;
    }

    const float distance = zombieX - plant->m_Transform.translation.x;
    if (distance < bestDistance) {
      bestDistance = distance;
      bestTarget = plant;
    }
  }

  return bestTarget;
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
    if (m_IsCherryBombDeath && m_CherryBombDyingAnimation != nullptr) {
      m_CherryBombDyingAnimation->SetCurrentFrame(0);
      m_CherryBombDyingAnimation->Play();
      SetDrawable(m_CherryBombDyingAnimation);
    } else if (m_DyingAnimation != nullptr) {
      m_DyingAnimation->SetCurrentFrame(0);
      m_DyingAnimation->Play();
      SetDrawable(m_DyingAnimation);
    }
    // Adjust scale to match target height
    {
      float heightForDeath = m_TargetHeightPx;
      if (m_IsCherryBombDeath) {
        if (m_CherryBombDeathTargetHeightPx > 0.0F) {
          heightForDeath = m_CherryBombDeathTargetHeightPx;
        } else if (m_DeathTargetHeightPx > 0.0F) {
          heightForDeath = m_DeathTargetHeightPx;
        }
      } else {
        if (m_DeathTargetHeightPx > 0.0F) {
          heightForDeath = m_DeathTargetHeightPx;
        }
      }

      if (heightForDeath > 0.0F) {
        const float drawableHeight =
            GetScaledSize().y / glm::max(m_Transform.scale.y, 0.0001F);
        if (drawableHeight > 0.0F) {
          const float scale = heightForDeath / drawableHeight;
          m_Transform.scale = {scale, scale};
        }
      }

      // If polevaulting zombie died from cherry bomb, shift death animation
      // down
      if (m_IsCherryBombDeath) {
        if (dynamic_cast<PolevaultingZombie *>(this) != nullptr) {
          // shift down by 30% of object's height
          m_Transform.translation.y -= GetScaledSize().y * 0.30F;
        }
      }
    }
    break;
  }
}
