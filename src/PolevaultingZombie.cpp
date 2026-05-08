#include "PolevaultingZombie.hpp"

#include <limits>

#include "Plant.hpp"
#include "Util/Animation.hpp"

namespace {
// Custom AABB collision check for the polevaulting zombie's attack/eat range.
bool CheckAABBCollisionCustom(const Util::GameObject &a,
                              const Util::GameObject &b) {
  const glm::vec2 aSize = a.GetScaledSize();
  const glm::vec2 bSize = b.GetScaledSize();

  const glm::vec2 aCenter = a.m_Transform.translation;
  const glm::vec2 bCenter = b.m_Transform.translation;

  const float objectLeft = aCenter.x - aSize.x * 0.5F;
  const float collisionLeft = objectLeft + aSize.x * 0.5F;   // 50% position
  const float collisionRight = objectLeft + aSize.x * 0.75F; // 75% position
  const float collisionHeight = aSize.y * 0.80F;
  const float collisionTop = aCenter.y + collisionHeight * 0.5F;
  const float collisionBottom = aCenter.y - collisionHeight * 0.5F;

  const glm::vec2 aMin = {collisionLeft, collisionBottom};
  const glm::vec2 aMax = {collisionRight, collisionTop};
  const glm::vec2 bMin = bCenter - bSize * 0.5F;
  const glm::vec2 bMax = bCenter + bSize * 0.5F;

  const bool overlapX = aMin.x <= bMax.x && aMax.x >= bMin.x;
  const bool overlapY = aMin.y <= bMax.y && aMax.y >= bMin.y;
  return overlapX && overlapY;
}

// Narrow collision check centered at 40% x for jump trigger
bool CheckJumpCollisionAt40(const Util::GameObject &a,
                            const Util::GameObject &b) {
  const glm::vec2 aSize = a.GetScaledSize();
  const glm::vec2 bSize = b.GetScaledSize();

  const glm::vec2 aCenter = a.m_Transform.translation;
  const glm::vec2 bCenter = b.m_Transform.translation;

  const float objectLeft = aCenter.x - aSize.x * 0.5F;
  const float centerPos = objectLeft + aSize.x * 0.40F; // 40% position
  const float stripHalfWidth =
      aSize.x * 0.025F; // narrow strip (2.5% either side)

  const float collisionTop = aCenter.y + aSize.y * 0.5F;
  const float collisionBottom = aCenter.y - aSize.y * 0.5F;

  const glm::vec2 aMin = {centerPos - stripHalfWidth, collisionBottom};
  const glm::vec2 aMax = {centerPos + stripHalfWidth, collisionTop};
  const glm::vec2 bMin = bCenter - bSize * 0.5F;
  const glm::vec2 bMax = bCenter + bSize * 0.5F;

  const bool overlapX = aMin.x <= bMax.x && aMax.x >= bMin.x;
  const bool overlapY = aMin.y <= bMax.y && aMax.y >= bMin.y;
  return overlapX && overlapY;
}

std::shared_ptr<Plant>
FindCollidingPlantInternal(const Util::GameObject &zombie,
                           const std::vector<std::shared_ptr<Plant>> &plants) {
  std::shared_ptr<Plant> bestTarget = nullptr;
  float bestDistance = std::numeric_limits<float>::max();
  const float zombieX = zombie.m_Transform.translation.x;

  for (const auto &plant : plants) {
    if (plant == nullptr || plant->IsDead()) {
      continue;
    }

    if (plant->m_Transform.translation.x > zombieX) {
      continue;
    }

    if (!CheckAABBCollisionCustom(zombie, *plant)) {
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

std::shared_ptr<Plant> FindJumpCollidingPlantInternal(
    const Util::GameObject &zombie,
    const std::vector<std::shared_ptr<Plant>> &plants) {
  std::shared_ptr<Plant> bestTarget = nullptr;
  float bestDistance = std::numeric_limits<float>::max();
  const float zombieX = zombie.m_Transform.translation.x;

  for (const auto &plant : plants) {
    if (plant == nullptr || plant->IsDead()) {
      continue;
    }

    if (plant->m_Transform.translation.x > zombieX) {
      continue;
    }

    if (!CheckJumpCollisionAt40(zombie, *plant)) {
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
} // namespace

PolevaultingZombie::PolevaultingZombie(
    const std::vector<std::string> &standFrames,
    const std::vector<std::string> &runFrames,
    const std::vector<std::string> &walkFrames,
    const std::vector<std::string> &attackingFrames,
    const std::vector<std::string> &jump1Frames,
    const std::vector<std::string> &jump2Frames,
    const std::vector<std::string> &dyingFrames,
    const std::vector<std::string> &cherryBombDyingFrames,
    const float targetHeightPx, const std::size_t frameIntervalMs,
    const float runSpeedPxPerSec, const float walkSpeedPxPerSec,
    const int health)
    : Zombie(runFrames, attackingFrames, dyingFrames, cherryBombDyingFrames,
             targetHeightPx, frameIntervalMs, walkSpeedPxPerSec, health),
      m_StandAnimation(std::make_shared<Util::Animation>(
          standFrames, true, frameIntervalMs, true, 0)),
      m_RunAnimation(std::make_shared<Util::Animation>(
          runFrames, true, frameIntervalMs, true, 0)),
      m_PostJumpWalkAnimation(std::make_shared<Util::Animation>(
          walkFrames, true, frameIntervalMs, true, 0)),
      m_Jump1Animation(std::make_shared<Util::Animation>(
          jump1Frames, true, frameIntervalMs, false, 0)),
      m_Jump2Animation(std::make_shared<Util::Animation>(
          jump2Frames, true, frameIntervalMs, false, 0)),
      m_RunSpeedPxPerSec(runSpeedPxPerSec),
      m_WalkSpeedPxPerSec(walkSpeedPxPerSec) {
  if (m_StandAnimation != nullptr) {
    SetAnimation(m_StandAnimation);
  } else if (m_RunAnimation != nullptr) {
    SetAnimation(m_RunAnimation);
    m_State = PolevaultState::Running;
  }
}

void PolevaultingZombie::Update(
    const float dt, const std::vector<std::shared_ptr<Plant>> &plants) {
  if (dt <= 0.0F || IsDestroyed()) {
    return;
  }

  if (GetState() == Zombie::State::Dying) {
    Zombie::Update(dt, plants);
    return;
  }

  switch (m_State) {
  case PolevaultState::Stand: {
    m_StandElapsed += dt;
    const auto target = FindCollidingPlant(plants);
    if (target != nullptr || m_StandElapsed >= 0.25F) {
      EnterRunning();
    }
    break;
  }

  case PolevaultState::Running: {
    m_Transform.translation.x -= m_RunSpeedPxPerSec * 2.0F * dt;
    // Use 40% jump trigger collision when deciding to start jump
    const auto target = FindJumpCollidingPlantInternal(*this, plants);
    if (target != nullptr) {
      m_CurrentTarget = target;
      EnterJump1();
    }
    break;
  }

  case PolevaultState::Jump1: {
    if (m_Jump1Animation != nullptr &&
        m_Jump1Animation->GetState() == Util::Animation::State::ENDED) {
      EnterJump2();
    }
    break;
  }

  case PolevaultState::Jump2: {
    if (m_Jump2Animation != nullptr &&
        m_Jump2Animation->GetState() == Util::Animation::State::ENDED) {
      m_HasJumped = true;
      EnterWalking();
    }
    break;
  }

  case PolevaultState::Walking: {
    m_Transform.translation.x -= m_WalkSpeedPxPerSec * dt;
    const auto target = FindCollidingPlant(plants);
    if (target != nullptr) {
      m_CurrentTarget = target;
      EnterAttacking();
    }
    break;
  }

  case PolevaultState::Attacking: {
    if (m_CurrentTarget == nullptr || m_CurrentTarget->IsDead() ||
        !CheckAABBCollisionCustom(*this, *m_CurrentTarget)) {
      m_CurrentTarget = FindCollidingPlant(plants);
      if (m_CurrentTarget == nullptr) {
        EnterWalking();
        break;
      }
    }

    m_AttackElapsed += dt;
    if (m_AttackElapsed >= m_AttackIntervalSec) {
      m_CurrentTarget->TakeDamage(m_AttackDamage);
      m_AttackElapsed = 0.0F;

      if (m_CurrentTarget->IsDead()) {
        m_CurrentTarget = nullptr;
        EnterWalking();
      }
    }
    break;
  }
  }
}

std::shared_ptr<Plant> PolevaultingZombie::FindCollidingPlant(
    const std::vector<std::shared_ptr<Plant>> &plants) const {
  return FindCollidingPlantInternal(*this, plants);
}

void PolevaultingZombie::SetAnimation(
    const std::shared_ptr<Util::Animation> &animation, const bool play) {
  if (animation == nullptr) {
    return;
  }

  animation->SetCurrentFrame(0);
  if (play) {
    animation->Play();
  }
  SetDrawable(animation);
}

void PolevaultingZombie::EnterStand() {
  m_State = PolevaultState::Stand;
  m_StandElapsed = 0.0F;
  SetAnimation(m_StandAnimation);
}

void PolevaultingZombie::EnterRunning() {
  m_State = PolevaultState::Running;
  SetAnimation(m_RunAnimation);
}

void PolevaultingZombie::EnterJump1() {
  m_State = PolevaultState::Jump1;
  SetAnimation(m_Jump1Animation);
}

void PolevaultingZombie::EnterJump2() {
  m_State = PolevaultState::Jump2;

  // Move horizontally by 0.5x the animation width when transitioning
  if (m_Jump2Animation != nullptr) {
    const float animWidth = m_Jump2Animation->GetSize().x;
    m_Transform.translation.x -= animWidth * 0.5F;
  }

  SetAnimation(m_Jump2Animation);
}

void PolevaultingZombie::EnterWalking() {
  m_State = PolevaultState::Walking;
  SetAnimation(m_PostJumpWalkAnimation);
}

void PolevaultingZombie::EnterAttacking() {
  m_State = PolevaultState::Attacking;
  m_AttackElapsed = 0.0F;
  if (m_AttackingAnimation != nullptr) {
    SetDrawable(m_AttackingAnimation);
  }
}
