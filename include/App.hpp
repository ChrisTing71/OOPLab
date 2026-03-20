#ifndef APP_HPP
#define APP_HPP

#include <random>

#include "pch.hpp" // IWYU pragma: export

#include "BasicZombie.hpp"
#include "CardSlot.hpp"
#include "Peashooter.hpp"
#include "Sun.hpp"
#include "Sunflower.hpp"
#include "Util/Renderer.hpp"

class App {
public:
  enum class State {
    START,
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

  struct ActiveSun {
    std::shared_ptr<Sun> object;
    std::weak_ptr<Sunflower> producer;
    float aliveSeconds = 0.0F;
    float stoppedSeconds = 0.0F;
    bool collecting = false;
    float collectElapsed = 0.0F;
    glm::vec2 collectStart = {0.0F, 0.0F};
    bool rising = false;
    float riseElapsed = 0.0F;
    glm::vec2 riseStart = {0.0F, 0.0F};
    glm::vec2 riseTarget = {0.0F, 0.0F};
    bool falling = true;
    bool stopped = false;
    float stopLocalY = 0.0F;
    bool expires = true;
    bool fromSky = false;
  };

  enum class PlantCardSelection {
    NONE,
    SUNFLOWER,
    PEASHOOTER,
  };

private:
  void ValidTask();
  bool PrepareFramesFromGif(const std::string &gifPath,
                            const std::string &framesDir,
                            const std::string &framePrefix,
                            std::vector<std::string> &framePaths,
                            int &frameIntervalMs);
  void UpdateCamera(float deltaTime);
  bool PrepareSunflowerFrames();
  bool PreparePeashooterFrames();
  bool PrepareBasicZombieFrames();
  bool PreparePlantPlacement(int row, int column, int &index,
                             glm::vec2 &localPosition,
                             float &targetHeight) const;
  bool PlaceSunflowerAtGridCell(int row, int column);
  bool PlacePeashooterAtGridCell(int row, int column);
  void SetupPlantCards();
  bool TrySelectPlantCardAt(float pixelX, float pixelY);
  void UpdateSelectedPlantPreview();
  bool IsCellOccupied(int index) const;
  glm::vec2 ComputeGridCellLocalPosition(int row, int column) const;
  float ComputeGridCellTargetHeight() const;
  void SpawnFallingSun();
  void SpawnSunFromSunflower(const std::shared_ptr<Sunflower> &sunflower);
  void UpdateSuns(float deltaTime);
  void SetupBasicZombieStand();
  void UpdateBasicZombie(float deltaTime);
  bool TryCollectSunAt(float pixelX, float pixelY);
  glm::vec2 CardSlotLocalFromSourceCoord(float sourceX, float sourceY) const;
  glm::vec2 ScreenPercentToRootLocal(float xPercent, float yPercent) const;
  float GridRowCenterPercent(int row) const;
  std::vector<std::shared_ptr<Plant>> CollectAlivePlants() const;
  void RemoveSunAt(std::size_t index);
  void DrawSunlightCounter() const;

private:
  static constexpr float kGridMinXPercent = 21.0F;
  static constexpr float kGridMaxXPercent = 89.0F;
  static constexpr float kGridMinYPercent = 14.0F;
  static constexpr float kGridMaxYPercent = 98.0F;
  static constexpr int kGridColumns = 9;
  static constexpr int kGridRows = 5;
  static constexpr int kGridCellCount = kGridColumns * kGridRows;

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
  std::vector<std::string> m_PeashooterFramePaths;
  int m_PeashooterFrameIntervalMs = 100;
  std::array<std::shared_ptr<Sunflower>, kGridCellCount> m_Sunflowers{};
  std::array<std::shared_ptr<Peashooter>, kGridCellCount> m_Peashooters{};

  std::vector<std::string> m_BasicZombieStandFramePaths;
  int m_BasicZombieStandFrameIntervalMs = 120;
  std::vector<std::string> m_BasicZombieWalkFramePaths;
  int m_BasicZombieWalkFrameIntervalMs = 120;
  std::vector<std::string> m_BasicZombieEatFramePaths;
  int m_BasicZombieEatFrameIntervalMs = 120;
  std::vector<std::string> m_BasicZombieDeadFramePaths;
  int m_BasicZombieDeadFrameIntervalMs = 120;

  std::shared_ptr<Util::GameObject> m_BasicZombieStand =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<BasicZombie> m_BasicZombie = nullptr;
  bool m_BasicZombieStandReady = false;
  bool m_BasicZombieStartedWalking = false;
  int m_BasicZombieRow = 0;
  float m_BasicZombieStandYPercent = 0.0F;
  float m_BasicZombieMoveDelayCountdown = 15.0F;

  std::shared_ptr<CardSlot> m_CardSlot = std::make_shared<CardSlot>();
  std::shared_ptr<Util::GameObject> m_SunflowerCard =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_PeashooterCard =
      std::make_shared<Util::GameObject>();
  std::shared_ptr<Util::GameObject> m_SelectedPlantPreview =
      std::make_shared<Util::GameObject>();

  PlantCardSelection m_SelectedPlant = PlantCardSelection::NONE;
  Util::Renderer m_UIRoot;

  bool m_SunSystemStarted = false;
  float m_SunSpawnCountdown = 0.0F;
  std::vector<ActiveSun> m_Suns;
  int m_Sunlight = 0;
  std::mt19937 m_Random{std::random_device{}()};
};

#endif
