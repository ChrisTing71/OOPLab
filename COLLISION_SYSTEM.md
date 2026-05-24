# 統一碰撞系統文檔

## 概述

整個專案的碰撞系統已從分散的實現統一為一個中央化的 `CollisionSystem` 命名空間。所有碰撞檢測都使用 AABB（軸對齐邊界框）與可配置的碰撞盒參數。

## 碰撞盒類型

### 1. 植物 (Plant)
- **碰撞盒**: 中心點 10% × 10% 的大小 (scale: 0.1, 0.1)
- **中心點**: 對象中心點
- **應用**: 所有植物類型（向日葵、豌豆射手、堅果、櫻桃炸彈）
- **說明**: 植物的碰撞點是一個小的 10% × 10% 中心區域

### 2. 基礎殭屍 (BasicZombie)
- **碰撞盒**: 全對象大小 (scale: 1.0, 1.0)
- **中心點**: 對象中心點
- **應用**: 基礎殭屍與植物碰撞檢測

### 3. 錐頭殭屍 (ConeheadZombie)
- **碰撞盒**: 全對象大小 (scale: 1.0, 1.0)
- **中心點**: 對象中心點
- **應用**: 錐頭殭屍與植物碰撞檢測

### 4. 領導者殭屍 (LeaderZombie)
- **碰撞盒**: 全對象大小 (scale: 1.0, 1.0)
- **中心點**: 對象中心點
- **應用**: 領導者殭屍與植物碰撞檢測

### 5. 撐竿殭屍攻擊 (PolevaultingZombieAttack)
- **碰撞盒寬度**: 25% (50%-75% 的對象寬度)
- **碰撞盒高度**: 60% (0.60F)（底線以上）
- **中心點**: X 方向向右偏移 12.5%，Y 方向向下偏移 20%（在碰撞盒上方）
- **應用**: 撐竿殭屍攻擊植物時的碰撞檢測

### 6. 撐竿殭屍跳躍觸發 (PolevaultingZombieJumpTrigger)
- **碰撞盒寬度**: 5% (窄條帶)
- **碰撞盒高度**: 100% (全高度)
- **中心點**: X 方向在 40% 位置（左偏移 10% 寬度）
- **應用**: 撐竿殭屍跳過植物的觸發檢測

### 7. 豌豆射彈 (PeaProjectile)
- **碰撞盒**: 80% x 75% (去掉透明邊框)
- **中心點**: 對象中心點
- **應用**: 豌豆與殭屍碰撞檢測

### 8. 割草機 (LawnMower)
- **碰撞盒**: 
  - 武裝狀態: 80% x 75%
  - 活躍狀態: 85% x 75%
- **中心點**: 對象中心點
- **應用**: 割草機與殭屍碰撞檢測

### 9. 太陽 (Sun)
- **碰撞盒**: 全對象大小 (scale: 1.0, 1.0)
- **應用**: 點擊收集太陽的像素碰撞檢測

### 10. 櫻桃炸彈爆炸 (CherryBombExplosion)
- **碰撞方式**: 網格距離檢測
- **範圍**: 中心位置周圍 ±1 格（行和列）
- **應用**: 爆炸傷害所有相鄰格子中的殭屍

## 核心函數

### 1. CheckAABBCollision
```cpp
bool CheckAABBCollision(const Util::GameObject &a, 
                        const Util::GameObject &b,
                        CollisionBoxType typeA, 
                        CollisionBoxType typeB)
```
使用預定義的碰撞盒類型進行 AABB 碰撞檢測。

### 2. CheckCustomAABBCollision
```cpp
bool CheckCustomAABBCollision(const Util::GameObject &a,
                              const Util::GameObject &b,
                              const glm::vec2 aScale, 
                              const glm::vec2 bScale,
                              const glm::vec2 aOffset, 
                              const glm::vec2 bOffset)
```
使用自訂參數進行 AABB 碰撞檢測。允許靈活配置碰撞盒。

### 3. CheckPolevaultingZombieProjectileCollision
```cpp
bool CheckPolevaultingZombieProjectileCollision(
    const Util::GameObject &projectile, 
    const Util::GameObject &zombie)
```
特殊碰撞檢測：豌豆與撐竿殭屍的特殊碰撞盒（50%-75% 寬度）。

### 4. CheckPolevaultingZombieJumpTrigger
```cpp
bool CheckPolevaultingZombieJumpTrigger(
    const Util::GameObject &zombie,
    const Util::GameObject &plant)
```
特殊碰撞檢測：撐竿殭屍跳躍觸發（40% 位置的窄帶）。

### 5. IsPixelInsideObject
```cpp
bool IsPixelInsideObject(const std::shared_ptr<Util::GameObject> &object,
                         float pixelX, float pixelY)
```
檢查像素位置是否在對象內部。用於 UI 交互和太陽收集。

### 6. CheckCherryBombExplosionCollision
```cpp
bool CheckCherryBombExplosionCollision(
    int centerRow, int centerColumn,
    int zombieRow, int zombieColumn)
```
檢查殭屍是否在櫻桃炸彈爆炸範圍內（網格距離）。

## 實現位置

### 頭文件
- `include/CollisionSystem.hpp` - 碰撞系統介面和類型定義

### 實現文件
- `src/CollisionSystem.cpp` - 碰撞檢測函數實現

### 使用位置
- `src/App.cpp` - 割草機、豌豆、太陽、櫻桃炸彈碰撞
- `src/Zombie.cpp` - 殭屍與植物碰撞
- `src/PolevaultingZombie.cpp` - 撐竿殭屍特殊碰撞

## 使用示例

### 檢測兩個對象是否碰撞
```cpp
// 使用預定義類型
if (CollisionSystem::CheckAABBCollision(
        zombie, plant,
        CollisionSystem::CollisionBoxType::BasicZombie,
        CollisionSystem::CollisionBoxType::Plant)) {
    // 碰撞發生
}

// 使用自訂參數
if (CollisionSystem::CheckCustomAABBCollision(
        pea, zombie,
        glm::vec2(0.80F, 0.75F),  // pea scale
        glm::vec2(0.48F, 0.80F),  // zombie scale
        glm::vec2(0.0F, 0.0F),    // pea offset
        glm::vec2(-zombieWidth * 0.10F, 0.0F))) {  // zombie offset
    // 碰撞發生
}
```

### 檢查點擊位置
```cpp
if (CollisionSystem::IsPixelInsideObject(sunObject, pixelX, pixelY)) {
    // 太陽被點擊
}
```

### 撐竿殭屍特殊碰撞
```cpp
// 跳躍觸發檢測
if (CollisionSystem::CheckPolevaultingZombieJumpTrigger(zombie, plant)) {
    // 觸發跳躍
}

// 豌豆碰撞檢測
if (CollisionSystem::CheckPolevaultingZombieProjectileCollision(pea, zombie)) {
    // 豌豆擊中撐竿殭屍
}
```

## 碰撞盒計算方法

所有碰撞盒都使用以下公式計算：

```cpp
碰撞盒寬度 = 對象寬度 × scale.x
碰撞盒高度 = 對象高度 × scale.y
碰撞盒中心 = 對象中心 + (offset.x × 對象寬度, offset.y × 對象高度)
// 注：offset 值是百分比（-0.5 到 0.5），自動根據對象大小計算

AABB 碰撞檢測:
aMin = aCenter - aSize * 0.5F
aMax = aCenter + aSize * 0.5F
bMin = bCenter - bSize * 0.5F
bMax = bCenter + bSize * 0.5F

碰撞 = (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
       (aMin.y <= bMax.y && aMax.y >= bMin.y)
```

## 優勢

1. **統一接口**: 所有碰撞檢測都通過統一的命名空間
2. **易於維護**: 碰撞邏輯集中在一個文件中
3. **靈活配置**: 支持預定義類型和自訂參數
4. **可擴展性**: 易於添加新的碰撞盒類型
5. **現有數據**: 使用現有的對象尺寸和位置數據，無需額外參數

## 已解決的問題

- ✅ 植物的碰撞點改在 object 的中心點
- ✅ 殭屍使用定義的碰撞盒
- ✅ 割草機使用自身 object 碰撞盒
- ✅ 所有碰撞邏輯使用現有數據
- ✅ 統一的碰撞系統架構
