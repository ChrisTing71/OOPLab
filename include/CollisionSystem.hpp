#ifndef COLLISION_SYSTEM_HPP
#define COLLISION_SYSTEM_HPP

#include "pch.hpp"

#include "Util/GameObject.hpp"

/**
 * Unified collision system for all game objects.
 * All collisions use AABB (Axis-Aligned Bounding Box) with configurable
 * collision boxes.
 *
 * Collision Box Definition:
 * - Each object has a collision box defined by scale and offset
 * - Scale: 0.0-1.0 representing the percentage of the object's size
 * - Offset: pixel offset from the object's center
 * - Default (1.0, 1.0, 0, 0) means the full object size, centered at object's
 * center
 */
namespace CollisionSystem {

/**
 * Collision box types and parameters for each entity type.
 */
enum class CollisionBoxType {
  // Plants - all use center point as collision point
  Plant,

  // Zombies
  BasicZombie,
  ConeheadZombie,
  LeaderZombie,

  // Polevaulting Zombie has two collision types
  PolevaultingZombieAttack,      // 50%-75% of width for attack range
  PolevaultingZombieJumpTrigger, // 40% position for jump trigger

  // Projectiles
  PeaProjectile,

  // Lawn Mower
  LawnMower,

  // Sun - uses pixel collision
  Sun,

  // Cherry Bomb Explosion - uses grid-based collision
  CherryBombExplosion,
};

/**
 * Collision box data for a specific entity type.
 */
struct CollisionBox {
  glm::vec2 scale;  // Width and height scale (0.0-1.0)
  glm::vec2 offset; // Pixel offset from center
};

/**
 * World-space collision box bounds.
 */
struct CollisionBoxBounds {
  float minX;
  float maxX;
  float minY;
  float maxY;
};

/**
 * Get collision box parameters for a specific entity type.
 * @param type The collision box type
 * @return Collision box parameters (scale and offset)
 */
CollisionBox GetCollisionBox(CollisionBoxType type);

/**
 * Get world-space bounds for a collision box type.
 */
CollisionBoxBounds GetCollisionBoxBounds(const Util::GameObject &object,
                                         CollisionBoxType boxType);

/**
 * Check AABB collision between two game objects using their collision boxes.
 * @param a First game object
 * @param b Second game object
 * @param typeA Collision box type for object a
 * @param typeB Collision box type for object b
 * @return True if objects collide
 */
bool CheckAABBCollision(const Util::GameObject &a, const Util::GameObject &b,
                        CollisionBoxType typeA, CollisionBoxType typeB);

/**
 * Check if two objects are in the same gameplay row.
 */
bool AreInSameGridRow(const Util::GameObject &a, const Util::GameObject &b);

/**
 * Check AABB collision and require both objects to be in the same row.
 */
bool CheckAABBCollisionSameRow(const Util::GameObject &a,
                               const Util::GameObject &b,
                               CollisionBoxType typeA, CollisionBoxType typeB);

/**
 * Check AABB collision with explicit row values.
 * Use this for objects that are not Plant/Zombie but still have gameplay rows.
 */
bool CheckAABBCollisionSameRow(int rowA, int rowB, const Util::GameObject &a,
                               const Util::GameObject &b,
                               CollisionBoxType typeA, CollisionBoxType typeB);

/**
 * Check collision with custom collision box parameters (for flexible
 * collision).
 * @param a First game object
 * @param b Second game object
 * @param aScale Scale of collision box for a
 * @param bScale Scale of collision box for b
 * @param aOffset Offset of collision box for a
 * @param bOffset Offset of collision box for b
 * @return True if objects collide
 */
bool CheckCustomAABBCollision(const Util::GameObject &a,
                              const Util::GameObject &b, const glm::vec2 aScale,
                              const glm::vec2 bScale, const glm::vec2 aOffset,
                              const glm::vec2 bOffset);

/**
 * Special collision check for polevaulting zombie projectiles.
 * Uses custom collision box for polevaulting zombie.
 * @param projectile Pea projectile object
 * @param zombie Polevaulting zombie object
 * @return True if projectile hits the zombie's collision box
 */
bool CheckPolevaultingZombieProjectileCollision(
    const Util::GameObject &projectile, const Util::GameObject &zombie);

/**
 * Special collision check for polevaulting zombie jump trigger.
 * Checks narrow strip at 40% x position.
 * @param zombie Polevaulting zombie object
 * @param plant Plant object
 * @return True if jump trigger is activated
 */
bool CheckPolevaultingZombieJumpTrigger(const Util::GameObject &zombie,
                                        const Util::GameObject &plant);

/**
 * Check if a pixel position is inside an object's collision box.
 * Used for sun collection and UI interactions.
 * @param object Game object
 * @param pixelX Screen pixel X coordinate
 * @param pixelY Screen pixel Y coordinate
 * @return True if pixel is inside the object
 */
bool IsPixelInsideObject(const std::shared_ptr<Util::GameObject> &object,
                         float pixelX, float pixelY);

/**
 * Check grid-based collision for cherry bomb explosions.
 * Cherry bomb explosions affect all zombies in adjacent grid cells.
 * @param centerRow Row of the cherry bomb
 * @param centerColumn Column of the cherry bomb
 * @param zombieRow Row of the zombie
 * @param zombieColumn Column of the zombie
 * @return True if zombie is in explosion range
 */
bool CheckCherryBombExplosionCollision(int centerRow, int centerColumn,
                                       int zombieRow, int zombieColumn);

/**
 * Determine the appropriate CollisionBoxType for a zombie object.
 * Returns PolevaultingZombieAttack for PolevaultingZombie instances,
 * BasicZombie for all others. Centralises the dynamic_cast pattern that
 * was repeated at every bullet hit-detection call site.
 *
 * @param zombie  Any Zombie-derived object (as a Util::GameObject reference).
 */
CollisionBoxType ZombieCollisionBoxType(const Util::GameObject &zombie);

} // namespace CollisionSystem

#endif // COLLISION_SYSTEM_HPP
