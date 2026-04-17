# OOPLab 角色與功能 C++ 實作說明

本文件說明此專案中「每個角色」與「主要功能」如何在 C++ 中實作，並對應到實際檔案與方法。

## 1. 整體架構

- 入口點: `src/main.cpp`
- 遊戲主控制器: `include/App.hpp`, `src/App.cpp`
- 關卡與選單流程: `LevelManager`, `MenuScene`
- 角色(植物、殭屍、資源物件): `Plant`/`Zombie` 及其子類別

主程式在 while 迴圈中每幀呼叫 `app.Update()`，並依 `App::State` 切換流程。

## 2. 遊戲狀態機 (App)

`App` 是整個遊戲的協調者，透過狀態機管理流程:

- `START`: 初始化資源、建立 `LevelManager`/`MenuScene`
- `MENU`: 顯示關卡選擇
- `GAME_LOADING`: 載入關卡並重置場景
- `PLAYING`: 正常戰鬥更新
- `LEVEL_COMPLETE`: 過關 UI 與下一關/回選單
- `LEVEL_FAILED`: 失敗 UI 與重試/回選單
- `END`: 結束遊戲

主要實作在 `App::Update()`。

## 3. 角色類別設計

### 3.1 植物基底 Plant

檔案: `include/Plant.hpp`

- 繼承 `Util::GameObject`，所以可直接掛上 drawable 與 transform
- 管理共同生命值欄位 `m_Health`
- 提供通用方法:
  - `GetHealth()`
  - `IsDead()`
  - `TakeDamage(int)`

所有植物共用這套血量與死亡判斷。

### 3.2 向日葵 Sunflower

檔案: `include/Sunflower.hpp`, `src/Sunflower.cpp`

- 建構子載入動畫並依目標高度縮放
- `ShouldProduceSun(deltaTime)`:
  - 用倒數計時器控制產陽光時機
  - 若已有未收集陽光，暫不再產生
- `OnProducedSunCollected()`:
  - 收集後重置冷卻(24 秒)
- `GetSunSpawnOffset()`, `GetSunPopTargetOffset()`:
  - 回傳陽光出現點與上拋目標點，讓 `App` 管理位移動畫

### 3.3 豌豆射手 Peashooter

檔案: `include/Peashooter.hpp`, `src/Peashooter.cpp`

- 內部分為待機動畫與攻擊動畫
- `StartAttack(...)`:
  - 切換到單次攻擊動畫
- `UpdateAttackStateAndCheckShoot()`:
  - 到達特定影格(第 17 幀)時回傳 shouldShoot=true
  - 動畫結束後回到待機

`App` 透過這個回傳值決定何時 `SpawnPeaFromPeashooter()`。

### 3.4 堅果牆 Nut

檔案: `include/Nut.hpp`, `src/Nut.cpp`

- 高生命值(4000)
- 內含 4 套受損階段動畫
- 覆寫 `TakeDamage`:
  - 先走基底扣血
  - 再依剩餘血量切換階段貼圖 (`UpdateStageDrawableByHealth`)

這是典型「狀態外觀隨血量變化」的 C++ 物件封裝。

### 3.5 櫻桃炸彈 CherryBomb

檔案: `include/CherryBomb.hpp`, `src/CherryBomb.cpp`

- 有待機動畫 + 爆炸動畫
- `UpdateAndCheckExplode(deltaTime)`:
  - 待機動畫播完，切換爆炸動畫並回傳 true (代表這一幀要套用爆炸傷害)
  - 爆炸動畫播完即標記 finished 與死亡

`App::UpdateCherryBombs` 在接收到 true 時，對周圍 3x3 格殭屍套用高傷害。

### 3.6 陽光物件 Sun

檔案: `include/Sun.hpp`, `src/Sun.cpp`

- 單純顯示物件，載入 `Resources/ui/hud/sun.png`
- 由 `App::ActiveSun` 包裝其運動狀態:
  - 天降 falling/stopped
  - 向日葵彈出 rising
  - 被收集 collecting

### 3.7 殭屍基底 Zombie

檔案: `include/Zombie.hpp`, `src/Zombie.cpp`

- 狀態機 `Walking`, `Attacking`, `Dying`
- `Update(dt, plants)`:
  - Walking: 向左移動，檢查是否碰撞到植物
  - Attacking: 週期性對目標植物扣血
  - Dying: 播死亡動畫，結束後標記 destroyed
- `TakeDamage(amount, isCherryBombDamage)`:
  - 扣血到 0 進入 Dying
  - 櫻桃炸彈可切換另一組死亡動畫
- `CheckAABBCollision`:
  - AABB 碰撞檢測(矩形重疊)

### 3.8 基礎殭屍 BasicZombie

檔案: `include/BasicZombie.hpp`, `src/BasicZombie.cpp`

- 目前是 `Zombie` 的輕量封裝
- 透過建構子傳入速度、血量與影格資料
- 實際行為幾乎都沿用 `Zombie` 基底

### 3.9 卡槽 CardSlot

檔案: `include/CardSlot.hpp`, `src/CardSlot.cpp`

- UI 元件，載入上方卡槽圖片 `Resources/ui/hud/upper_slot.png`
- 保留原始 source size，讓 `App` 用來源座標換算 UI 位置

## 4. 選單與關卡

### 4.1 MenuScene

檔案: `include/MenuScene.hpp`, `src/MenuScene.cpp`

- 建立 10 關按鈕
- 支援鍵盤與滑鼠輸入
- `Update()` 回傳使用者所選 levelId
- `RenderLevelSelect()` 透過 ImGui 繪製選單

### 4.2 LevelConfig 與載入

檔案: `include/LevelConfig.hpp`, `src/LevelConfig.cpp`

- `LevelConfig` 描述關卡資料:
  - 初始陽光
  - 場景類型
  - 波次 phases
  - 獎勵
- `LevelConfigLoader::LoadFromFile` 解析 JSON
- `GetDefaultConfig` 提供 fallback

### 4.3 WaveConfig

檔案: `include/WaveConfig.hpp`, `src/WaveConfig.cpp`

- `WaveConfigLoader::LoadFromFile` 解析波次 JSON
- 逐 phase 驗證欄位合法性(不可負值、repeat>0 等)

### 4.4 LevelManager

檔案: `include/LevelManager.hpp`, `src/LevelManager.cpp`

- `LoadLevel(levelId)` 載入關卡設定
- 管理 `m_LevelCompleted`/`m_LevelFailed`
- `GetNextLevelId()`、`CanProgressToNextLevel()` 控制關卡推進

## 5. 核心功能如何在 C++ 實作

### 5.1 GIF 轉影格與動畫資產

實作: `App::PrepareFramesFromGif`

- 使用 SDL_image 的 animation API 讀 GIF
- 逐幀輸出 PNG 到 `Resources/.../frames`
- 平均 delay 推算 `frameIntervalMs`
- 後續 `Util::Animation` 直接吃這些影格路徑

### 5.2 攝影機導覽與進場

實作: `App::UpdateCamera`

- 分三段 stage:
  - 先停左側房子
  - 平移到右側道路
  - 回中間戰鬥位置
- 在 stage 完成時啟動:
  - 陽光系統
  - 波次系統
  - 卡槽與割草機顯示

### 5.3 格子座標轉換與種植

實作: `ComputeGridCellLocalPosition`, `HandleGridClick`, `PlaceXxxAtGridCell`

- 將螢幕百分比換算成 5x9 格 row/column
- 透過 `IsCellOccupied` 防止同格重複種植
- 植物成功放置後扣陽光

### 5.4 卡片 UI 與禁用遮罩

實作: `SetupPlantCards`, `PrepareGrayCardImage`, `UpdatePlantCardUIState`

- 每張卡有 normal 與 disabled 兩個 `GameObject`
- disabled 圖在啟動時動態產生灰階 PNG
- 陽光不足時顯示 disabled 層

### 5.5 陽光經濟循環

實作: `SpawnFallingSun`, `SpawnSunFromSunflower`, `UpdateSuns`, `TryCollectSunAt`

- 天降陽光與向日葵陽光共用 `ActiveSun` 狀態資料
- 點擊收集後，陽光會飛向卡槽計數區
- 收集完成增加 `m_Sunlight += 25`

### 5.6 殭屍生成與波次

實作: `BuildZombieSpawnPlan`, `UpdateBasicZombie`, `SpawnBasicZombieAtRow`

- 把 phase 資料展開成可執行的 spawn group 時間線
- 依 start delay、repeat、interval、waitUntilClear 控制出怪
- 每幀更新所有 active zombie 狀態

### 5.7 豌豆攻擊與碰撞

實作: `UpdatePeashooterCombat`

- 同排有殭屍才會進攻
- 開火節奏由 attack cooldown + 攻擊動畫影格控制
- 子彈飛行後與殭屍做 AABB 檢測
- 命中切 hit 動畫並對殭屍扣血

### 5.8 櫻桃炸彈 AoE

實作: `UpdateCherryBombs`

- 由 `CherryBomb::UpdateAndCheckExplode` 回傳爆炸時機
- 以炸彈格為中心，套用 row/column 距離 <= 1 的範圍判定
- 範圍內殭屍立即高傷害

### 5.9 割草機保底機制

實作: `SetupLawnMowers`, `UpdateLawnMowers`

- 每排一台割草機，預設 armed
- 偵測與該排殭屍接觸後啟動衝刺
- 移動過程持續清除同排殭屍
- 離開畫面後銷毀

### 5.10 勝敗條件

實作: `UpdateGameplay`, `App::Update`

- 失敗: 任一殭屍越過左側防線
- 勝利: 所有波次已生成完，且場上無存活殭屍

## 6. 物件與資料流總結

每幀更新大致順序:

1. `UpdateCamera`
2. 啟動/更新陽光系統
3. 更新殭屍、割草機、櫻桃炸彈、豌豆
4. 清理死亡植物
5. 更新卡片可用狀態
6. 處理輸入(收陽光、選卡、種植/鏟除)
7. Renderer 更新與 UI 顯示

這個順序確保「輸出畫面前，邏輯狀態已完整收斂」。

## 7. 若要新增角色，建議的 C++ 擴充步驟

1. 新增 `include/NewPlant.hpp` 與 `src/NewPlant.cpp`，繼承 `Plant` 或 `Zombie`
2. 在 `App` 補齊資源準備與容器欄位
3. 在放置/生成函式中建立該物件
4. 在每幀更新函式加入其行為邏輯
5. 在清理流程與碰撞流程加入對應處理
6. 更新卡片 UI 與成本(若是植物)

照這個模式可維持和現有架構一致，避免邏輯分散。
