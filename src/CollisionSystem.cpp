#include "CollisionSystem.hpp"

#include "Plant.hpp"
#include "Zombie.hpp"
#include "config.hpp"

namespace CollisionSystem {

namespace {
CollisionBoxBounds MakeBounds(const glm::vec2 &center, const glm::vec2 &size) {
  return {center.x - size.x * 0.5F, center.x + size.x * 0.5F,
          center.y - size.y * 0.5F, center.y + size.y * 0.5F};
}

CollisionBoxBounds GetStaticCollisionBoxBounds(const Util::GameObject &object,
                                               CollisionBoxType boxType) {
  const auto box = GetCollisionBox(boxType);
  const glm::vec2 objSize = object.GetScaledSize();
  const glm::vec2 boxSize = objSize * box.scale;

  // Offsets are percentages of the ORIGINAL (unscaled) object size
  const glm::vec2 originalSize =
      objSize / glm::vec2(glm::max(object.m_Transform.scale.x, 0.0001F),
                          glm::max(object.m_Transform.scale.y, 0.0001F));
  const glm::vec2 actualOffset =
      glm::vec2(box.offset.x * originalSize.x, box.offset.y * originalSize.y);
  const glm::vec2 boxCenter = object.m_Transform.translation + actualOffset;
  return MakeBounds(boxCenter, boxSize);
}

} // namespace

CollisionBox GetCollisionBox(CollisionBoxType type) {
  switch (type) {
  // All plants use a centered box.
  case CollisionBoxType::Plant:
    return {{0.10F, 0.80F}, {0.0F, 0.0F}};

  // Basic zombies share the same default box.
  case CollisionBoxType::BasicZombie:
  case CollisionBoxType::ConeheadZombie:
  case CollisionBoxType::LeaderZombie:
    return {{0.60F, 0.60F}, {0.0F, 0.0F}};

  // Polevaulting zombie attack box.
  case CollisionBoxType::PolevaultingZombieAttack:
    return {{0.15F, 0.60F}, {0.15F, -0.20F}};

  // Polevaulting zombie jump trigger box.
  case CollisionBoxType::PolevaultingZombieJumpTrigger:
    return {{0.10F, 1.0F}, {-0.15F, -0.3F}};

  // Pea projectile.
  case CollisionBoxType::PeaProjectile:
    return {{1.0F, 1.0F}, {0.0F, 0.0F}};

  // Lawn mower.
  case CollisionBoxType::LawnMower:
    return {{0.80F, 0.75F}, {0.0F, 0.0F}};

  // Sun.
  case CollisionBoxType::Sun:
    return {{1.0F, 1.0F}, {0.0F, 0.0F}};

  // Cherry bomb explosion.
  case CollisionBoxType::CherryBombExplosion:
    return {{1.0F, 1.0F}, {0.0F, 0.0F}};

  default:
    return {{1.0F, 1.0F}, {0.0F, 0.0F}};
  }
}

bool CheckAABBCollision(const Util::GameObject &a, const Util::GameObject &b,
                        CollisionBoxType typeA, CollisionBoxType typeB) {
  const auto boxA = GetCollisionBox(typeA);
  const auto boxB = GetCollisionBox(typeB);
  return CheckCustomAABBCollision(a, b, boxA.scale, boxB.scale, boxA.offset,
                                  boxB.offset);
}

bool AreInSameGridRow(const Util::GameObject &a, const Util::GameObject &b) {
  const auto *plantA = dynamic_cast<const Plant *>(&a);
  const auto *zombieA = dynamic_cast<const Zombie *>(&a);
  const auto *plantB = dynamic_cast<const Plant *>(&b);
  const auto *zombieB = dynamic_cast<const Zombie *>(&b);

  const int rowA = plantA != nullptr    ? plantA->GetGridRow()
                   : zombieA != nullptr ? zombieA->GetGridRow()
                                        : -1;
  const int rowB = plantB != nullptr    ? plantB->GetGridRow()
                   : zombieB != nullptr ? zombieB->GetGridRow()
                                        : -1;

  return rowA >= 0 && rowA == rowB;
}

bool CheckAABBCollisionSameRow(const Util::GameObject &a,
                               const Util::GameObject &b,
                               CollisionBoxType typeA, CollisionBoxType typeB) {
  return AreInSameGridRow(a, b) && CheckAABBCollision(a, b, typeA, typeB);
}

bool CheckAABBCollisionSameRow(int rowA, int rowB, const Util::GameObject &a,
                               const Util::GameObject &b,
                               CollisionBoxType typeA, CollisionBoxType typeB) {
  return rowA >= 0 && rowA == rowB && CheckAABBCollision(a, b, typeA, typeB);
}

bool CheckCustomAABBCollision(const Util::GameObject &a,
                              const Util::GameObject &b, const glm::vec2 aScale,
                              const glm::vec2 bScale, const glm::vec2 aOffset,
                              const glm::vec2 bOffset) {
  const glm::vec2 aSize = a.GetScaledSize() * aScale;
  const glm::vec2 bSize = b.GetScaledSize() * bScale;

  // Offsets are percentages of the ORIGINAL (unscaled) object size
  // GetScaledSize() is already scaled, so we divide by scale to get original
  // size
  const glm::vec2 aOriginalSize =
      a.GetScaledSize() / glm::vec2(glm::max(a.m_Transform.scale.x, 0.0001F),
                                    glm::max(a.m_Transform.scale.y, 0.0001F));
  const glm::vec2 bOriginalSize =
      b.GetScaledSize() / glm::vec2(glm::max(b.m_Transform.scale.x, 0.0001F),
                                    glm::max(b.m_Transform.scale.y, 0.0001F));

  const glm::vec2 actualOffsetA =
      glm::vec2(aOffset.x * aOriginalSize.x, aOffset.y * aOriginalSize.y);
  const glm::vec2 actualOffsetB =
      glm::vec2(bOffset.x * bOriginalSize.x, bOffset.y * bOriginalSize.y);
  const glm::vec2 aCenter = a.m_Transform.translation + actualOffsetA;
  const glm::vec2 bCenter = b.m_Transform.translation + actualOffsetB;

  const glm::vec2 aMin = aCenter - aSize * 0.5F;
  const glm::vec2 aMax = aCenter + aSize * 0.5F;
  const glm::vec2 bMin = bCenter - bSize * 0.5F;
  const glm::vec2 bMax = bCenter + bSize * 0.5F;

  const bool overlapX = aMin.x <= bMax.x && aMax.x >= bMin.x;
  const bool overlapY = aMin.y <= bMax.y && aMax.y >= bMin.y;
  return overlapX && overlapY;
}

CollisionBoxBounds GetCollisionBoxBounds(const Util::GameObject &object,
                                         CollisionBoxType boxType) {
  return GetStaticCollisionBoxBounds(object, boxType);
}

bool CheckPolevaultingZombieProjectileCollision(
    const Util::GameObject &projectile, const Util::GameObject &zombie) {
  return CheckAABBCollision(projectile, zombie, CollisionBoxType::PeaProjectile,
                            CollisionBoxType::PolevaultingZombieAttack);
}

bool CheckPolevaultingZombieJumpTrigger(const Util::GameObject &zombie,
                                        const Util::GameObject &plant) {
  return CheckAABBCollisionSameRow(
      zombie, plant, CollisionBoxType::PolevaultingZombieJumpTrigger,
      CollisionBoxType::Plant);
}

bool IsPixelInsideObject(const std::shared_ptr<Util::GameObject> &object,
                         float pixelX, float pixelY) {
  if (object == nullptr) {
    return false;
  }

  const glm::vec2 size = object->GetScaledSize();
  const glm::vec2 center = {
      object->m_Transform.translation.x +
          static_cast<float>(WINDOW_WIDTH) * 0.5F,
      static_cast<float>(WINDOW_HEIGHT) * 0.5F -
          object->m_Transform.translation.y,
  };

  const float halfWidth = size.x * 0.5F;
  const float halfHeight = size.y * 0.5F;
  return (pixelX >= center.x - halfWidth) && (pixelX <= center.x + halfWidth) &&
         (pixelY >= center.y - halfHeight) && (pixelY <= center.y + halfHeight);
}

bool CheckCherryBombExplosionCollision(int centerRow, int centerColumn,
                                       int zombieRow, int zombieColumn) {
  // Cherry bomb explosions affect zombies in adjacent grid cells
  // (within 1 cell distance in both row and column)
  return glm::abs(zombieRow - centerRow) <= 1 &&
         glm::abs(zombieColumn - centerColumn) <= 1;
}

} // namespace CollisionSystem
