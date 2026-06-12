#ifndef APP_HPP
#define APP_HPP

#include <random>
#include <set>

#include "pch.hpp" // IWYU pragma: export

#include "Bullet.hpp"
#include "CardSlot.hpp"
#include "SunManager.hpp"
#include "ZombieWaveController.hpp"
#include "CherryBomb.hpp"
#include "Fumeshroom.hpp"
#include "LevelManager.hpp"
#include "MenuScene.hpp"
#include "Nut.hpp"
#include "Peashooter.hpp"
#include "Plant.hpp"
#include "Puffshroom.hpp"
#include "Sun.hpp"
#include "Sunflower.hpp"
#include "Sunshroom.hpp"
#include "Util/Image.hpp"
#include "Util/Renderer.hpp"
#include "WaveConfig.hpp"
#include "Zombie.hpp"

class App {
public:
  enum class State {
    START,
    MENU,           // Main menu - level selection
    GAME_LOADING,   // Loading level resources
    PLAYING,        // Actively playing the level
    PAUSED,         // Game paused (pause menu visible)
    LEVEL_COMPLETE, // Level successfully completed
    LEVEL_FAILED,   // Level failed (zombies reached end)
    GAME_OVER,      // Game over
    UPDATE,
    END,
  };

  State GetCurrentState() const { return m_CurrentState; }

  void Start();

  void Update();

  void End(); // NOLINT(readability-convert-member-functions-to-static)

  enum class CameraStage {
    STAGE1_HOME,
    STAGE2_RIGHT,
    STAGE3_CENTER,
    FINISHED,
  };

  // Bullet collections for the two directed-projectile plant types.
  // Separated so each combat-update function processes only its own bullets.

  struct ActiveFumeshroomEffect {
    std::shared_ptr<Util::GameObject> object;
    int row = -1;
    float elapsedSec = 0.0F;
    std::set<Zombie *> damagedZombies;
    constexpr static float kDurationSec = 1.0F;
  };

  struct ActiveZombie {
    std::shared_ptr<Zombie> object;
  };

  struct LawnMower {
    std::shared_ptr<Util::GameObject> object;
    std::shared_ptr<Util::Animation> animation;
    int row = 0;
    bool armed = true;
    bool active = false;
    bool destroyed = false;
  };

  enum class PlantCardSelection {
    NONE,
    SUNFLOWER,
    SUNSHROOM,
    PUFFSHROOM,
    FUMESHROOM,
    PEASHOOTER,
    NUT,
    CHERRY_BOMB,
    SHOVEL,
  };

  struct PlantCardUI {
    PlantCardSelection selection = PlantCardSelection::NONE;
    int cost = 0;
    std::shared_ptr<Util::GameObject> normal = nullptr;
    std::shared_ptr<Util::GameObject> disabled = nullptr;
    std::shared_ptr<Util::Image> normalImage = nullptr;
    std::shared_ptr<Util::Image> disabledImage = nullptr;
    std::string normalImagePath;
    std::string disabledImagePath;
    float cooldownRemaining = 0.0F; // Remaining cooldown in seconds
    float cooldownTotal = 7.5F;     // Total cooldown time for this card
  };

private:
  void ValidTask();
  void ResetLevelRuntimeState();
  void InitializeLevel();
  void UpdateGameplay(float deltaTime);
  bool PrepareFramesFromGif(const std::string &gifPath,
                            const std::string &framesDir,
                            const std::string &framePrefix,
                            std::vector<std::string> &framePaths,
                            int &frameIntervalMs);
  bool PrepareGrayCardImage(const std::string &sourcePath,
                            const std::string &outputPath);
  void UpdateCamera(float deltaTime);
  bool PrepareSunflowerFrames();
  bool PrepareSunshroomFrames();
  bool PreparePeashooterFrames();
  bool PreparePuffshroomFrames();
  bool PrepareFumeshroomFrames();
  bool PrepareFumeshroomAttackFrames();
  bool PrepareFumeshroomBulletFrames();
  bool PrepareNutFrames();
  bool PrepareCherryBombFrames();
  bool PrepareCherryBombBlowFrames();
  bool PreparePeashooterAttackFrames();
  bool PrepareBasicZombieFrames();
  bool PrepareLeaderZombieFrames();
  bool PrepareConeheadZombieFrames();
  bool PreparePolevaultingZombieFrames();
  bool PrepareLawnMowerFrames();
  bool PreparePlantPlacement(int row, int column, int &index,
                             glm::vec2 &localPosition) const;
  bool PlaceSunflowerAtGridCell(int row, int column);
  bool PlaceSunshroomAtGridCell(int row, int column);
  bool PlacePeashooterAtGridCell(int row, int column);
  bool PlacePuffshroomAtGridCell(int row, int column);
  bool PlaceFumeshroomAtGridCell(int row, int column);
  bool PlaceNutAtGridCell(int row, int column);
  bool PlaceCherryBombAtGridCell(int row, int column);
  bool RemovePlantAtGridCell(int row, int column);
  void SetupPlantCards();
  bool TrySelectPlantCardAt(float pixelX, float pixelY);
  void UpdateSelectedPlantPreview();
  bool IsCellOccupied(int index) const;
  glm::vec2 ComputeGridCellLocalPosition(int row, int column) const;
  float ComputeGridCellTargetHeight() const;
  float ComputePlantTargetHeight() const;
  float ComputeZombieTargetHeight() const;
  float ComputePeaTargetHeight() const;
  float ComputePlantPreviewTargetHeight() const;
  void PrepareBasicZombieStandPreview();
  void SetupBasicZombieStand();
  void ClearBasicZombieStandPreview();
  int GetPlannedZombieCount() const;
  void SpawnZombieAtRow(int row, const std::string &zombieType);
  int PickSpawnRowForWaveSpawn();
  bool HasAliveZombie() const;
  const std::vector<std::string> &
  GetZombiePreviewStandFramePaths(const std::string &zombieType) const;
  int GetZombiePreviewStandFrameIntervalMs(const std::string &zombieType) const;
  float ComputeZombiePreviewTargetHeight(const std::string &zombieType) const;
  void UpdateSuns(float deltaTime);
  bool TryCollectSunAt(float pixelX, float pixelY);
  void UpdateBasicZombie(float deltaTime);
  void SetupLawnMowers();
  void UpdateLawnMowers(float deltaTime);
  void UpdateCherryBombs(float deltaTime);
  void UpdatePeashooterCombat(float deltaTime);
  void UpdatePuffshroomCombat(float deltaTime);
  void UpdateFumeshroomCombat(float deltaTime);
  bool PrepareShroomBulletFrames();
  bool PrepareShroomBulletHitFrames();
  void SpawnPeaFromPeashooter(const std::shared_ptr<Peashooter> &peashooter);
  void SpawnShroomBulletFromPuffshroom(const std::shared_ptr<Puffshroom> &puff);
  void SpawnFumeshroomAttackEffect(const std::shared_ptr<Fumeshroom> &fume);
  bool HasAliveZombieInRow(int row, float shooterX) const;
  void RemoveDeadPlants();
  void UpdatePlantCardUIState();
  void DrawGameplayCheatToggle();
  void ClearSelectedPlantTool();
  void HandleGridClick(float xPercent, float yPercent, bool collectedSun,
                       bool selectedCard);
  void StartPlantCardCooldown(PlantCardSelection sel);
  glm::vec2 CardSlotLocalFromSourceCoord(float sourceX, float sourceY) const;
  glm::vec2 ScreenPercentToRootLocal(float xPercent, float yPercent) const;
  float GridRowCenterPercent(int row) const;
  std::vector<std::shared_ptr<Plant>> CollectAlivePlants() const;
  void DrawSunlightCounter() const;
  void DrawPauseButton();
  void DebugDrawMouseOverlay() const;
  void DebugDrawCollisionBoxes() const;
  void SetupBannerObject(const std::shared_ptr<Util::GameObject> &banner,
                         const std::string &imagePath, float zIndex,
                         bool fullScreen) const;
  void TriggerHugeWaveBanner();
  void TriggerGameOverBanner();
  void UpdateBannerState(float deltaTime);

private:
  static constexpr float kGridMinXPercent = 21.0F;
  static constexpr float kGridMaxXPercent = 89.0F;
  static constexpr float kGridMinYPercent = 14.0F;
  static constexpr float kGridMaxYPercent = 98.0F;
  static constexpr int kGridColumns = 9;
  static constexpr int kGridRows = 5;
  static constexpr int kGridCellCount = kGridColumns * kGridRows;

  static constexpr int kSunflowerCost = 50;
  static constexpr int kSunshroomCost = 25;
  static constexpr int kFumeshroomCost = 75;
  static constexpr int kPeashooterCost = 100;
  static constexpr int kNutCost = 50;
  static constexpr int kCherryBombCost = 150;

  // Zombie animation size scale factors
  static constexpr float kBasicZombieHeightScale = 1.0F;
  static constexpr float kLeaderZombieHeightScale = 1.3F;
  static constexpr float kConeheadZombieHeightScale = 1.2F;
  static constexpr float kPolevaultingZombieHeightScale = 1.7F;

private:
  State m_CurrentState = State::START;
  std::shared_ptr<Util::GameObject> m_Map =
      std::make_shared<Util::GameObject>();
  Util::Renderer m_Root;

  CameraStage m_CameraStage = CameraStage::STAGE1_HOME;
  float m_CameraStageElapsed = 0.0F;
  float m_CameraFromX = 0.0F;
  float m_CameraToX = 0.0F;
  float m_CameraCurrentX = 0.0F;

  float m_MapScaledWidth = 0.0F;
  bool m_CameraInitialized = false;

  bool m_HasClickedPoint = false;
  bool m_HasGridHit = false;
  glm::vec2 m_LastClickPixel = {0.0F, 0.0F};
  glm::vec2 m_LastClickPercent = {0.0F, 0.0F};
  int m_LastHitRow = 0;
  int m_LastHitColumn = 0;

  std::vector<std::string> m_SunflowerFramePaths;
  int m_SunflowerFrameIntervalMs = 100;
  std::vector<std::string> m_SunshroomInitialFramePaths;
  int m_SunshroomInitialFrameIntervalMs = 100;
  std::vector<std::string> m_SunshroomGrownFramePaths;
  int m_SunshroomGrownFrameIntervalMs = 100;
  std::vector<std::string> m_PeashooterFramePaths;
  int m_PeashooterFrameIntervalMs = 100;
  std::vector<std::string> m_PuffshroomFramePaths;
  int m_PuffshroomFrameIntervalMs = 100;
  std::vector<std::string> m_FumeshroomFramePaths;
  int m_FumeshroomFrameIntervalMs = 100;
  std::vector<std::string> m_FumeshroomAttackFramePaths;
  int m_FumeshroomAttackFrameIntervalMs = 100;
  std::vector<std::string> m_FumeshroomBulletFramePaths;
  int m_FumeshroomBulletFrameIntervalMs = 100;
  std::vector<std::string> m_Nut1FramePaths;
  int m_Nut1FrameIntervalMs = 100;
  std::vector<std::string> m_Nut2FramePaths;
  int m_Nut2FrameIntervalMs = 100;
  std::vector<std::string> m_Nut3FramePaths;
  int m_Nut3FrameIntervalMs = 100;
  std::vector<std::string> m_Nut4FramePaths;
  int m_Nut4FrameIntervalMs = 100;
  std::vector<std::string> m_CherryBombFramePaths;
  int m_CherryBombFrameIntervalMs = 100;
  std::vector<std::string> m_CherryBombBlowFramePaths;
  int m_CherryBombBlowFrameIntervalMs = 100;
  std::vector<std::string> m_PeashooterAttackFramePaths;
  int m_PeashooterAttackFrameIntervalMs = 100;
  std::array<float, kGridCellCount> m_PeashooterAttackCooldowns{};
  std::array<float, kGridCellCount> m_PuffshroomAttackCooldowns{};
  std::array<float, kGridCellCount> m_PuffshroomAttackWarmupRemaining{};
  std::array<float, kGridCellCount> m_FumeshroomAttackCooldowns{};
  std::array<float, kGridCellCount> m_FumeshroomAttackWarmupRemaining{};
  std::vector<std::string> m_ShroomBulletFramePaths;
  int m_ShroomBulletFrameIntervalMs = 100;
  std::vector<std::string> m_ShroomBulletHitFramePaths;
  int m_ShroomBulletHitFrameIntervalMs = 100;
  std::vector<std::shared_ptr<Bullet>> m_PeaBullets;
  std::vector<std::shared_ptr<Bullet>> m_ShroomBullets;
  std::vector<ActiveFumeshroomEffect> m_FumeshroomEffects;
  std::vector<std::string> m_PeaHitFramePaths = {
      "Resources/gameplay/plants/peashooter/peashooter_bullet/hit1.png",
      "Resources/gameplay/plants/peashooter/peashooter_bullet/hit2.png",
      "Resources/gameplay/plants/peashooter/peashooter_bullet/hit3.png",
      "Resources/gameplay/plants/peashooter/peashooter_bullet/hit4.png",
  };
  std::array<std::shared_ptr<Sunflower>, kGridCellCount> m_Sunflowers{};
  std::array<std::shared_ptr<Sunshroom>, kGridCellCount> m_Sunshrooms{};
  std::array<std::shared_ptr<Puffshroom>, kGridCellCount> m_Puffshrooms{};
  std::array<std::shared_ptr<Fumeshroom>, kGridCellCount> m_Fumeshrooms{};
  std::array<std::shared_ptr<Peashooter>, kGridCellCount> m_Peashooters{};
  std::array<std::shared_ptr<Nut>, kGridCellCount> m_Nuts{};
  std::array<std::shared_ptr<CherryBomb>, kGridCellCount> m_CherryBombs{};

  std::vector<std::string> m_BasicZombieStandFramePaths;
  int m_BasicZombieStandFrameIntervalMs = 120;
  std::vector<std::string> m_LeaderZombieStandFramePaths;
  int m_LeaderZombieStandFrameIntervalMs = 120;
  std::vector<std::string> m_BasicZombieWalkFramePaths;
  int m_BasicZombieWalkFrameIntervalMs = 120;
  std::vector<std::string> m_BasicZombieEatFramePaths;
  int m_BasicZombieEatFrameIntervalMs = 120;
  std::vector<std::string> m_BasicZombieDeadFramePaths;
  int m_BasicZombieDeadFrameIntervalMs = 120;
  std::vector<std::string> m_LeaderZombieWalkFramePaths;
  int m_LeaderZombieWalkFrameIntervalMs = 120;
  std::vector<std::string> m_LeaderZombieEatFramePaths;
  int m_LeaderZombieEatFrameIntervalMs = 120;
  std::vector<std::string> m_LeaderZombieDeadFramePaths;
  int m_LeaderZombieDeadFrameIntervalMs = 120;
  std::vector<std::string> m_ConeheadZombieStandFramePaths;
  int m_ConeheadZombieStandFrameIntervalMs = 120;
  std::vector<std::string> m_ConeheadZombieWalkFramePaths;
  int m_ConeheadZombieWalkFrameIntervalMs = 120;
  std::vector<std::string> m_ConeheadZombieEatFramePaths;
  int m_ConeheadZombieEatFrameIntervalMs = 120;
  std::vector<std::string> m_ConeheadZombieDeadFramePaths;
  int m_ConeheadZombieDeadFrameIntervalMs = 120;
  std::vector<std::string> m_PolevaultingZombieStandFramePaths;
  int m_PolevaultingZombieStandFrameIntervalMs = 120;
  std::vector<std::string> m_PolevaultingZombieRunFramePaths;
  int m_PolevaultingZombieRunFrameIntervalMs = 120;
  std::vector<std::string> m_PolevaultingZombieJump1FramePaths;
  int m_PolevaultingZombieJump1FrameIntervalMs = 120;
  std::vector<std::string> m_PolevaultingZombieJump2FramePaths;
  int m_PolevaultingZombieJump2FrameIntervalMs = 120;
  std::vector<std::string> m_PolevaultingZombieWalkFramePaths;
  int m_PolevaultingZombieWalkFrameIntervalMs = 120;
  std::vector<std::string> m_PolevaultingZombieEatFramePaths;
  int m_PolevaultingZombieEatFrameIntervalMs = 120;
  std::vector<std::string> m_PolevaultingZombieDeadFramePaths;
  int m_PolevaultingZombieDeadFrameIntervalMs = 120;
  std::vector<std::string> m_LawnMowerFramePaths;
  int m_LawnMowerFrameIntervalMs = 100;
  std::vector<std::string> m_CherryBombDeadFramePaths;

  std::vector<std::shared_ptr<Util::GameObject>> m_BasicZombieStands;
  std::vector<glm::vec2> m_BasicZombieStandPercents;
  std::vector<ActiveZombie> m_ActiveZombies;
  std::array<LawnMower, kGridRows> m_LawnMowers{};
  float m_GameOverBoundaryX = kGridMinXPercent;
  bool m_BasicZombieStandReady = false;
  bool m_UseStandRowForNextSpawn = true;
  float m_BasicZombieStandYPercent = 0.0F;
  LevelWaveConfig m_LevelWaveConfig;
  ZombieWaveController m_WaveController;
  bool m_HasShownHugeWaveBanner = false;
  float m_HugeWaveBannerRemainingSec = 0.0F;
  float m_GameOverBannerRemainingSec = 0.0F;

  std::shared_ptr<CardSlot> m_CardSlot = std::make_shared<CardSlot>();
  std::shared_ptr<Util::GameObject> m_SunflowerCard =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_SunshroomCard =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_PeashooterCard =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_PuffshroomCard =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_FumeshroomCard =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_NutCard =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_CherryBombCard =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_SunflowerCardGrayMask =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_SunshroomCardGrayMask =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_PeashooterCardGrayMask =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_PuffshroomCardGrayMask =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_FumeshroomCardGrayMask =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_NutCardGrayMask =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_CherryBombCardGrayMask =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_ShovelShell =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_Shovel =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_SelectedPlantPreview =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_HugeWaveBanner =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_GameOverBanner =
      std::make_shared<Util::GameObject>();
  std::vector<PlantCardUI> m_PlantCards;

  bool m_CheatEnabled = false;
  PlantCardSelection m_SelectedPlant = PlantCardSelection::NONE;
  Util::Renderer m_UIRoot;

  int m_Sunlight = 0;
  mutable std::mt19937 m_Random{std::random_device{}()};
  SunManager m_SunManager{m_UIRoot, m_Random};

  // Level and Menu Management
  std::shared_ptr<LevelManager> m_LevelManager;
  std::shared_ptr<MenuScene> m_MenuScene;
  int m_SelectedLevelId = 1;
};

#endif
