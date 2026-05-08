#include "App.hpp"

#include <filesystem>
#include <random>

#include "BasicZombie.hpp"
#include "config.hpp"

#include "ConeheadZombie.hpp"
#include "PolevaultingZombie.hpp"

#include "LeaderZombie.hpp"

#include "Util/Animation.hpp"
#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Time.hpp"

namespace {
float Lerp(const float from, const float to, const float t) {
  return from + (to - from) * glm::clamp(t, 0.0F, 1.0F);
}

bool CheckCustomAABBCollision(const Util::GameObject &a,
                              const Util::GameObject &b, const glm::vec2 aScale,
                              const glm::vec2 bScale, const glm::vec2 aOffset,
                              const glm::vec2 bOffset) {
  const glm::vec2 aSize = a.GetScaledSize() * aScale;
  const glm::vec2 bSize = b.GetScaledSize() * bScale;
  const glm::vec2 aCenter = a.m_Transform.translation + aOffset;
  const glm::vec2 bCenter = b.m_Transform.translation + bOffset;

  const glm::vec2 aMin = aCenter - aSize * 0.5F;
  const glm::vec2 aMax = aCenter + aSize * 0.5F;
  const glm::vec2 bMin = bCenter - bSize * 0.5F;
  const glm::vec2 bMax = bCenter + bSize * 0.5F;

  const bool overlapX = aMin.x <= bMax.x && aMax.x >= bMin.x;
  const bool overlapY = aMin.y <= bMax.y && aMax.y >= bMin.y;
  return overlapX && overlapY;
}

bool IsPixelInsideObject(const std::shared_ptr<Util::GameObject> &object,
                         const float pixelX, const float pixelY) {
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
} // namespace

bool App::PrepareFramesFromGif(const std::string &gifPath,
                               const std::string &framesDir,
                               const std::string &framePrefix,
                               std::vector<std::string> &framePaths,
                               int &frameIntervalMs) {
  if (!framePaths.empty()) {
    return true;
  }

  std::error_code mkdirError;
  std::filesystem::create_directories(framesDir, mkdirError);

  IMG_Animation *gif = IMG_LoadAnimation(gifPath.c_str());
  if (gif == nullptr || gif->count <= 0) {
    if (gif != nullptr) {
      IMG_FreeAnimation(gif);
    }
    return false;
  }

  framePaths.reserve(static_cast<std::size_t>(gif->count));
  int delaySum = 0;
  int delayCount = 0;

  for (int i = 0; i < gif->count; ++i) {
    const std::string framePath =
        fmt::format("{}/{}_{:03d}.png", framesDir, framePrefix, i);
    if (IMG_SavePNG(gif->frames[i], framePath.c_str()) != 0) {
      continue;
    }

    framePaths.push_back(framePath);

    if (gif->delays != nullptr && gif->delays[i] > 0) {
      delaySum += gif->delays[i];
      ++delayCount;
    }
  }

  if (delayCount > 0) {
    frameIntervalMs = glm::max(30, delaySum / delayCount);
  }

  IMG_FreeAnimation(gif);
  return !framePaths.empty();
}

bool App::PrepareGrayCardImage(const std::string &sourcePath,
                               const std::string &outputPath) {
  if (std::filesystem::exists(outputPath)) {
    return true;
  }

  std::error_code mkdirError;
  const auto parent = std::filesystem::path(outputPath).parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, mkdirError);
  }

  SDL_Surface *source = IMG_Load(sourcePath.c_str());
  if (source == nullptr) {
    return false;
  }

  SDL_Surface *rgba =
      SDL_ConvertSurfaceFormat(source, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(source);
  if (rgba == nullptr) {
    return false;
  }

  for (int y = 0; y < rgba->h; ++y) {
    auto *row = static_cast<Uint32 *>(static_cast<void *>(
        static_cast<Uint8 *>(rgba->pixels) + y * rgba->pitch));
    for (int x = 0; x < rgba->w; ++x) {
      Uint8 r = 0;
      Uint8 g = 0;
      Uint8 b = 0;
      Uint8 a = 0;
      SDL_GetRGBA(row[x], rgba->format, &r, &g, &b, &a);
      const Uint8 gray = static_cast<Uint8>((r * 30 + g * 59 + b * 11) / 100);
      const Uint8 darkGray = static_cast<Uint8>(gray * 0.55F);
      row[x] = SDL_MapRGBA(rgba->format, darkGray, darkGray, darkGray, a);
    }
  }

  const int saveResult = IMG_SavePNG(rgba, outputPath.c_str());
  SDL_FreeSurface(rgba);
  if (saveResult != 0) {
    return false;
  }

  return true;
}

bool App::IsCellOccupied(const int index) const {
  return m_Sunflowers[static_cast<std::size_t>(index)] != nullptr ||
         m_Peashooters[static_cast<std::size_t>(index)] != nullptr ||
         m_Nuts[static_cast<std::size_t>(index)] != nullptr ||
         m_CherryBombs[static_cast<std::size_t>(index)] != nullptr;
}

glm::vec2 App::ComputeGridCellLocalPosition(const int row,
                                            const int column) const {
  const float cellWidthPercent =
      (kGridMaxXPercent - kGridMinXPercent) / static_cast<float>(kGridColumns);
  const float cellHeightPercent =
      (kGridMaxYPercent - kGridMinYPercent) / static_cast<float>(kGridRows);

  const float centerXPercent =
      kGridMinXPercent + (static_cast<float>(column) + 0.5F) * cellWidthPercent;
  const float centerYPercent =
      kGridMinYPercent + (static_cast<float>(row) + 0.5F) * cellHeightPercent;

  const float centerPixelX =
      (centerXPercent / 100.0F) * static_cast<float>(WINDOW_WIDTH);
  const float centerPixelY =
      (centerYPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);

  const float centerCursorX =
      centerPixelX - static_cast<float>(WINDOW_WIDTH) * 0.5F;
  const float centerCursorY =
      static_cast<float>(WINDOW_HEIGHT) * 0.5F - centerPixelY;

  constexpr float kCameraOffsetY = 0.05F * static_cast<float>(WINDOW_HEIGHT);
  return glm::vec2(centerCursorX, centerCursorY) -
         glm::vec2(m_CameraCurrentX, kCameraOffsetY);
}

float App::ComputeGridCellTargetHeight() const {
  const float cellHeightPercent =
      (kGridMaxYPercent - kGridMinYPercent) / static_cast<float>(kGridRows);
  const float cellHeightPixel =
      (cellHeightPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  return cellHeightPixel * 0.7F;
}

float App::ComputePlantTargetHeight() const {
  return ComputeGridCellTargetHeight();
}

float App::ComputeZombieTargetHeight() const {
  return ComputeGridCellTargetHeight() * 1.5F;
}

float App::ComputePeaTargetHeight() const {
  return ComputeGridCellTargetHeight() * 0.22F;
}

float App::ComputePlantPreviewTargetHeight() const {
  return ComputePlantTargetHeight() * 0.9F;
}

bool App::PrepareSunflowerFrames() {
  return PrepareFramesFromGif("Resources/gameplay/plants/sunflower.gif",
                              "Resources/gameplay/plants/sunflower_frames",
                              "sunflower_frame", m_SunflowerFramePaths,
                              m_SunflowerFrameIntervalMs);
}

bool App::PreparePeashooterFrames() {
  return PrepareFramesFromGif("Resources/gameplay/plants/peashooter.gif",
                              "Resources/gameplay/plants/peashooter_frames",
                              "peashooter_frame", m_PeashooterFramePaths,
                              m_PeashooterFrameIntervalMs);
}

bool App::PrepareNutFrames() {
  const bool ok1 = PrepareFramesFromGif(
      "Resources/gameplay/plants/wall_nut/nut1/Mobile - Plants vs. Zombies 2 - "
      "Wall-nut - Idle.gif",
      "Resources/gameplay/plants/wall_nut/nut1/frames", "nut1_frame",
      m_Nut1FramePaths, m_Nut1FrameIntervalMs);
  const bool ok2 = PrepareFramesFromGif(
      "Resources/gameplay/plants/wall_nut/nut2/Mobile - Plants vs. Zombies 2 - "
      "Wall-nut - Idle - Degrade 1.gif",
      "Resources/gameplay/plants/wall_nut/nut2/frames", "nut2_frame",
      m_Nut2FramePaths, m_Nut2FrameIntervalMs);
  const bool ok3 = PrepareFramesFromGif(
      "Resources/gameplay/plants/wall_nut/nut3/Mobile - Plants vs. Zombies 2 - "
      "Wall-nut - Idle - Degrade 2.gif",
      "Resources/gameplay/plants/wall_nut/nut3/frames", "nut3_frame",
      m_Nut3FramePaths, m_Nut3FrameIntervalMs);
  const bool ok4 = PrepareFramesFromGif(
      "Resources/gameplay/plants/wall_nut/nut4/Mobile - Plants vs. Zombies 2 - "
      "Wall-nut - Idle - Degrade 3.gif",
      "Resources/gameplay/plants/wall_nut/nut4/frames", "nut4_frame",
      m_Nut4FramePaths, m_Nut4FrameIntervalMs);
  return ok1 && ok2 && ok3 && ok4;
}

bool App::PrepareCherryBombFrames() {
  return PrepareFramesFromGif(
      "Resources/gameplay/plants/cherry_bomb/cherryBomb.gif",
      "Resources/gameplay/plants/cherry_bomb/cherryBomb_frames",
      "cherry_bomb_frame", m_CherryBombFramePaths, m_CherryBombFrameIntervalMs);
}

bool App::PrepareCherryBombBlowFrames() {
  return PrepareFramesFromGif(
      "Resources/gameplay/plants/cherry_bomb/blow.gif",
      "Resources/gameplay/plants/cherry_bomb/blow_frames",
      "cherry_bomb_blow_frame", m_CherryBombBlowFramePaths,
      m_CherryBombBlowFrameIntervalMs);
}

bool App::PreparePeashooterAttackFrames() {
  return PrepareFramesFromGif(
      "Resources/gameplay/plants/peashooter_attack/Mobile - Plants vs. "
      "Zombies 2 - Peashooter - Attack.gif",
      "Resources/gameplay/plants/peashooter_attack/frames",
      "peashooter_attack_frame", m_PeashooterAttackFramePaths,
      m_PeashooterAttackFrameIntervalMs);
}

bool App::PrepareBasicZombieFrames() {
  const bool okStand = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/basic_zombie/stand.gif",
      "Resources/gameplay/enemies/zombies/basic_zombie/stand_frames",
      "stand_frame", m_BasicZombieStandFramePaths,
      m_BasicZombieStandFrameIntervalMs);
  const bool okWalk = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/basic_zombie/walk.gif",
      "Resources/gameplay/enemies/zombies/basic_zombie/walk_frames",
      "walk_frame", m_BasicZombieWalkFramePaths,
      m_BasicZombieWalkFrameIntervalMs);
  const bool okEat = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/basic_zombie/eat.gif",
      "Resources/gameplay/enemies/zombies/basic_zombie/eat_frames", "eat_frame",
      m_BasicZombieEatFramePaths, m_BasicZombieEatFrameIntervalMs);
  const bool okDead = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/basic_zombie/dead.gif",
      "Resources/gameplay/enemies/zombies/basic_zombie/dead_frames",
      "dead_frame", m_BasicZombieDeadFramePaths,
      m_BasicZombieDeadFrameIntervalMs);
  const bool okCherryBombDead = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/cherry_bomb_dead.gif",
      "Resources/gameplay/enemies/zombies/cherry_bomb_dead_frames",
      "cherry_bomb_dead_frame", m_CherryBombDeadFramePaths,
      m_BasicZombieDeadFrameIntervalMs);
  return okStand && okWalk && okEat && okDead && okCherryBombDead;
}

bool App::PrepareLeaderZombieFrames() {
  const bool okStand = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/leader_zombie/"
      "Mobile - Plants vs. Zombies 2 - Basic Zombie - Idle - Flag Zombie.gif",
      "Resources/gameplay/enemies/zombies/leader_zombie/stand_frames",
      "leader_stand_frame", m_LeaderZombieStandFramePaths,
      m_LeaderZombieStandFrameIntervalMs);
  const bool okWalk = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/leader_zombie/"
      "Mobile - Plants vs. Zombies 2 - Basic Zombie - Walking - Flag "
      "Zombie.gif",
      "Resources/gameplay/enemies/zombies/leader_zombie/walk_frames",
      "leader_walk_frame", m_LeaderZombieWalkFramePaths,
      m_LeaderZombieWalkFrameIntervalMs);
  const bool okEat = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/leader_zombie/"
      "Mobile - Plants vs. Zombies 2 - Basic Zombie - Eating - Flag Zombie.gif",
      "Resources/gameplay/enemies/zombies/leader_zombie/eat_frames",
      "leader_eat_frame", m_LeaderZombieEatFramePaths,
      m_LeaderZombieEatFrameIntervalMs);
  const bool okDead = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/leader_zombie/"
      "Mobile - Plants vs. Zombies 2 - Basic Zombie - Death - Flag Zombie.gif",
      "Resources/gameplay/enemies/zombies/leader_zombie/dead_frames",
      "leader_dead_frame", m_LeaderZombieDeadFramePaths,
      m_LeaderZombieDeadFrameIntervalMs);
  return okStand && okWalk && okEat && okDead;
}

bool App::PrepareConeheadZombieFrames() {
  const bool okStand = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/conehead_zombie/stand.gif",
      "Resources/gameplay/enemies/zombies/conehead_zombie/stand_frames",
      "conehead_stand_frame", m_ConeheadZombieStandFramePaths,
      m_ConeheadZombieStandFrameIntervalMs);
  const bool okWalk = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/conehead_zombie/walk.gif",
      "Resources/gameplay/enemies/zombies/conehead_zombie/walk_frames",
      "conehead_walk_frame", m_ConeheadZombieWalkFramePaths,
      m_ConeheadZombieWalkFrameIntervalMs);
  const bool okEat = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/conehead_zombie/eat.gif",
      "Resources/gameplay/enemies/zombies/conehead_zombie/eat_frames",
      "conehead_eat_frame", m_ConeheadZombieEatFramePaths,
      m_ConeheadZombieEatFrameIntervalMs);
  const bool okDead = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/conehead_zombie/dead.gif",
      "Resources/gameplay/enemies/zombies/conehead_zombie/dead_frames",
      "conehead_dead_frame", m_ConeheadZombieDeadFramePaths,
      m_ConeheadZombieDeadFrameIntervalMs);
  return okStand && okWalk && okEat && okDead;
}

bool App::PreparePolevaultingZombieFrames() {
  const bool okStand = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/stand.gif",
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/stand_frames",
      "polevaulting_stand_frame", m_PolevaultingZombieStandFramePaths,
      m_PolevaultingZombieStandFrameIntervalMs);
  const bool okRun = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/run.gif",
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/run_frames",
      "polevaulting_run_frame", m_PolevaultingZombieRunFramePaths,
      m_PolevaultingZombieRunFrameIntervalMs);
  const bool okJump1 = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/jump1.gif",
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/jump1_frames",
      "polevaulting_jump1_frame", m_PolevaultingZombieJump1FramePaths,
      m_PolevaultingZombieJump1FrameIntervalMs);
  const bool okJump2 = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/jump2.gif",
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/jump2_frames",
      "polevaulting_jump2_frame", m_PolevaultingZombieJump2FramePaths,
      m_PolevaultingZombieJump2FrameIntervalMs);
  const bool okWalk = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/walk.gif",
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/walk_frames",
      "polevaulting_walk_frame", m_PolevaultingZombieWalkFramePaths,
      m_PolevaultingZombieWalkFrameIntervalMs);
  const bool okEat = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/eat.gif",
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/eat_frames",
      "polevaulting_eat_frame", m_PolevaultingZombieEatFramePaths,
      m_PolevaultingZombieEatFrameIntervalMs);
  const bool okDead = PrepareFramesFromGif(
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/dead.gif",
      "Resources/gameplay/enemies/zombies/polevaulting_zombie/dead_frames",
      "polevaulting_dead_frame", m_PolevaultingZombieDeadFramePaths,
      m_PolevaultingZombieDeadFrameIntervalMs);
  return okStand && okRun && okJump1 && okJump2 && okWalk && okEat && okDead;
}

bool App::PrepareLawnMowerFrames() {
  return PrepareFramesFromGif("Resources/gameplay/defense/lawn_mower/car.gif",
                              "Resources/gameplay/defense/lawn_mower/frames",
                              "car_frame", m_LawnMowerFramePaths,
                              m_LawnMowerFrameIntervalMs);
}

void App::BuildZombieSpawnPlan(const LevelWaveConfig &waveConfig) {
  m_ZombieWavePlan.clear();
  float timelineSec = 0.0F;

  for (const auto &phase : waveConfig.phases) {
    timelineSec += phase.startDelaySec;
    for (int i = 0; i < phase.repeat; ++i) {
      ZombieWavePhaseConfig phaseCopy = phase;
      if (phaseCopy.zombieTypes.empty()) {
        phaseCopy.zombieTypes = {phaseCopy.zombieType};
      }
      phaseCopy.zombieTypes = BuildZombieTypeSequence(phaseCopy);
      m_ZombieWavePlan.push_back({
          phase.id,
          phase.type,
          phase.zombieType,
          phaseCopy.zombieTypes,
          phaseCopy.randomOrder,
          timelineSec,
          phase.zombiesPerWave,
          phase.spawnIntervalSec,
          phase.waitUntilClear,
      });
      timelineSec += phase.waveIntervalSec;
    }
  }

  m_CurrentWaveGroupIndex = 0;
  m_WaveGroupActive = false;
  m_WaveGroupSpawnedCount = 0;
  m_WaveGroupSpawnTimer = 0.0F;
}

int App::GetPlannedZombieCount() const {
  int totalZombies = 0;
  for (const auto &waveGroup : m_ZombieWavePlan) {
    totalZombies += glm::max(0, waveGroup.zombieCount);
  }
  return totalZombies;
}

std::vector<std::string>
App::BuildZombieTypeSequence(const ZombieWavePhaseConfig &phase) const {
  std::vector<std::string> sequence;
  sequence.reserve(static_cast<std::size_t>(phase.zombiesPerWave));

  if (!phase.zombieTypes.empty()) {
    for (int index = 0; index < phase.zombiesPerWave; ++index) {
      sequence.push_back(phase.zombieTypes[static_cast<std::size_t>(index) %
                                           phase.zombieTypes.size()]);
    }
  } else {
    const std::string zombieType =
        phase.zombieType.empty() ? "basic" : phase.zombieType;
    for (int index = 0; index < phase.zombiesPerWave; ++index) {
      sequence.push_back(zombieType);
    }
  }

  if (phase.randomOrder && sequence.size() > 1) {
    std::shuffle(sequence.begin(), sequence.end(), m_Random);
  }

  return sequence;
}

const std::vector<std::string> &
App::GetZombiePreviewStandFramePaths(const std::string &zombieType) const {
  if (zombieType == "leader") {
    if (!m_LeaderZombieStandFramePaths.empty()) {
      return m_LeaderZombieStandFramePaths;
    }
  }
  if (zombieType == "conehead") {
    if (!m_ConeheadZombieStandFramePaths.empty()) {
      return m_ConeheadZombieStandFramePaths;
    }
  }
  if (zombieType == "polevaulting") {
    if (!m_PolevaultingZombieStandFramePaths.empty()) {
      return m_PolevaultingZombieStandFramePaths;
    }
  }
  return m_BasicZombieStandFramePaths;
}

int App::GetZombiePreviewStandFrameIntervalMs(
    const std::string &zombieType) const {
  if (zombieType == "leader") {
    return m_LeaderZombieStandFrameIntervalMs;
  }
  if (zombieType == "conehead") {
    return m_ConeheadZombieStandFrameIntervalMs;
  }
  if (zombieType == "polevaulting") {
    return m_PolevaultingZombieStandFrameIntervalMs;
  }
  return m_BasicZombieStandFrameIntervalMs;
}

float App::ComputeZombiePreviewTargetHeight(
    const std::string &zombieType) const {
  if (zombieType == "leader") {
    return ComputeZombieTargetHeight() * kLeaderZombieHeightScale;
  }
  if (zombieType == "conehead") {
    return ComputeZombieTargetHeight() * kConeheadZombieHeightScale;
  }
  if (zombieType == "polevaulting") {
    return ComputeZombieTargetHeight() * kPolevaultingZombieHeightScale;
  }
  return ComputeZombieTargetHeight() * kBasicZombieHeightScale;
}

void App::ClearBasicZombieStandPreview() {
  for (const auto &stand : m_BasicZombieStands) {
    if (stand != nullptr) {
      m_Root.RemoveChild(stand);
    }
  }
  m_BasicZombieStands.clear();
  m_BasicZombieStandPercents.clear();
  m_BasicZombieStandReady = false;
  m_UseStandRowForNextSpawn = true;
}

void App::PrepareBasicZombieStandPreview() {
  if (m_BasicZombieStandFramePaths.empty() &&
      m_LeaderZombieStandFramePaths.empty() &&
      m_ConeheadZombieStandFramePaths.empty()) {
    return;
  }

  const int zombieCount = GetPlannedZombieCount();
  if (zombieCount <= 0) {
    return;
  }

  ClearBasicZombieStandPreview();
  m_BasicZombieStands.reserve(static_cast<std::size_t>(zombieCount));
  m_BasicZombieStandPercents.reserve(static_cast<std::size_t>(zombieCount));

  const float halfVisibleWidth = static_cast<float>(WINDOW_WIDTH) * 0.5F;
  const float halfMapWidth = m_MapScaledWidth * 0.5F;
  const float panLimit = glm::max(0.0F, halfMapWidth - halfVisibleWidth);
  const float rightCameraX = -panLimit;

  const auto toRightCameraLocal = [&](const float xPercent,
                                      const float yPercent) {
    const float pixelX = (xPercent / 100.0F) * static_cast<float>(WINDOW_WIDTH);
    const float pixelY =
        (yPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);

    const float cursorX = pixelX - static_cast<float>(WINDOW_WIDTH) * 0.5F;
    const float cursorY = static_cast<float>(WINDOW_HEIGHT) * 0.5F - pixelY;

    constexpr float kCameraOffsetY = 0.05F * static_cast<float>(WINDOW_HEIGHT);
    return glm::vec2(cursorX, cursorY) -
           glm::vec2(rightCameraX, kCameraOffsetY);
  };

  std::uniform_real_distribution<float> xDist(75.0F, 80.0F);
  const float cellHeightPercent =
      (kGridMaxYPercent - kGridMinYPercent) / static_cast<float>(kGridRows);
  std::vector<float> yCandidates;
  yCandidates.reserve(static_cast<std::size_t>(kGridRows * 2));
  for (int step = 0; step <= (kGridRows - 1) * 2; ++step) {
    const float yPercent =
        kGridMinYPercent +
        (0.5F + 0.5F * static_cast<float>(step)) * cellHeightPercent;
    yCandidates.push_back(yPercent);
  }
  std::uniform_int_distribution<int> yIndexDist(
      0, static_cast<int>(yCandidates.size()) - 1);
  for (const auto &waveGroup : m_ZombieWavePlan) {
    for (int index = 0; index < waveGroup.zombieCount; ++index) {
      const float xPercent = xDist(m_Random);
      const float yPercent =
          yCandidates[static_cast<std::size_t>(yIndexDist(m_Random))];
      const std::string previewType =
          waveGroup.zombieTypes.empty()
              ? waveGroup.zombieType
              : waveGroup.zombieTypes[static_cast<std::size_t>(index) %
                                      waveGroup.zombieTypes.size()];

      const auto &standFrames = GetZombiePreviewStandFramePaths(previewType);
      const int standFrameIntervalMs =
          GetZombiePreviewStandFrameIntervalMs(previewType);
      const float previewTargetHeight =
          ComputeZombiePreviewTargetHeight(previewType);

      auto stand = std::make_shared<Util::GameObject>();
      auto standAnim = std::make_shared<Util::Animation>(
          standFrames, true, static_cast<std::size_t>(standFrameIntervalMs),
          true, 0);
      stand->SetDrawable(standAnim);
      const float yNormalized = glm::clamp(
          (yPercent - kGridMinYPercent) / (kGridMaxYPercent - kGridMinYPercent),
          0.0F, 1.0F);
      stand->SetZIndex(0.8F + yNormalized * 0.6F);
      stand->SetVisible(true);
      stand->m_Transform.translation = toRightCameraLocal(xPercent, yPercent);

      const glm::vec2 standSize = standAnim->GetSize();
      if (standSize.y > 0.0F) {
        const float scale = previewTargetHeight / standSize.y;
        stand->m_Transform.scale = {scale, scale};
      }

      if (m_BasicZombieStands.empty()) {
        m_BasicZombieStandYPercent = yPercent;
      }

      m_BasicZombieStandPercents.push_back({xPercent, yPercent});
      m_Root.AddChild(stand);
      m_BasicZombieStands.push_back(stand);
    }
  }
}

void App::SpawnZombieAtRow(const int row, const std::string &zombieType) {
  const float cellHeightPercent =
      (kGridMaxYPercent - kGridMinYPercent) / static_cast<float>(kGridRows);
  const float yPercent = GridRowCenterPercent(row) - cellHeightPercent * 0.1F;
  const glm::vec2 spawnPos = ScreenPercentToRootLocal(104.0F, yPercent);

  const std::string normalizedZombieType =
      zombieType.empty() ? std::string("basic") : zombieType;
  const bool isLeaderZombie = normalizedZombieType == "leader";
  const bool isConeheadZombie = normalizedZombieType == "conehead";
  const bool isPolevaultingZombie = normalizedZombieType == "polevaulting";

  const std::vector<std::string> &walkingFrames =
      isLeaderZombie && !m_LeaderZombieWalkFramePaths.empty()
          ? m_LeaderZombieWalkFramePaths
      : isConeheadZombie && !m_ConeheadZombieWalkFramePaths.empty()
          ? m_ConeheadZombieWalkFramePaths
      : isPolevaultingZombie && !m_PolevaultingZombieRunFramePaths.empty()
          ? m_PolevaultingZombieRunFramePaths
          : m_BasicZombieWalkFramePaths;
  const std::vector<std::string> &attackingFrames =
      isLeaderZombie && !m_LeaderZombieEatFramePaths.empty()
          ? m_LeaderZombieEatFramePaths
      : isConeheadZombie && !m_ConeheadZombieEatFramePaths.empty()
          ? m_ConeheadZombieEatFramePaths
      : isPolevaultingZombie && !m_PolevaultingZombieEatFramePaths.empty()
          ? m_PolevaultingZombieEatFramePaths
          : m_BasicZombieEatFramePaths;
  const std::vector<std::string> &dyingFrames =
      isLeaderZombie && !m_LeaderZombieDeadFramePaths.empty()
          ? m_LeaderZombieDeadFramePaths
      : isConeheadZombie && !m_ConeheadZombieDeadFramePaths.empty()
          ? m_ConeheadZombieDeadFramePaths
      : isPolevaultingZombie && !m_PolevaultingZombieDeadFramePaths.empty()
          ? m_PolevaultingZombieDeadFramePaths
          : m_BasicZombieDeadFramePaths;

  const float targetHeight = ComputeZombieTargetHeight() *
                             (isLeaderZombie           ? kLeaderZombieHeightScale
                              : isConeheadZombie        ? kConeheadZombieHeightScale
                              : isPolevaultingZombie    ? kPolevaultingZombieHeightScale
                                                          : kBasicZombieHeightScale);

  std::shared_ptr<Zombie> zombie;
  if (isLeaderZombie) {
    zombie = std::make_shared<LeaderZombie>(
        walkingFrames, attackingFrames, dyingFrames, m_CherryBombDeadFramePaths,
        targetHeight,
        static_cast<std::size_t>(m_BasicZombieWalkFrameIntervalMs), 17.0F, 200);
  } else if (isConeheadZombie) {
    zombie = std::make_shared<ConeheadZombie>(
        walkingFrames, attackingFrames, dyingFrames, m_CherryBombDeadFramePaths,
        targetHeight,
        static_cast<std::size_t>(m_ConeheadZombieWalkFrameIntervalMs), 17.0F,
        600);
    // Use 1.0x scale for death animation instead of 1.2x
    zombie->SetDeathTargetHeightPx(ComputeZombieTargetHeight() *
                                   kBasicZombieHeightScale);
  } else if (isPolevaultingZombie) {
    zombie = std::make_shared<PolevaultingZombie>(
        m_PolevaultingZombieStandFramePaths,
        m_PolevaultingZombieRunFramePaths,
        m_PolevaultingZombieWalkFramePaths,
        attackingFrames,
        m_PolevaultingZombieJump1FramePaths,
        m_PolevaultingZombieJump2FramePaths,
        dyingFrames,
        m_CherryBombDeadFramePaths,
        targetHeight,
        static_cast<std::size_t>(m_PolevaultingZombieRunFrameIntervalMs),
        17.0F,
        17.0F,
        370);
  } else {
    zombie = std::make_shared<BasicZombie>(
        walkingFrames, attackingFrames, dyingFrames, m_CherryBombDeadFramePaths,
        targetHeight,
        static_cast<std::size_t>(m_BasicZombieWalkFrameIntervalMs), 17.0F, 200);
  }
  zombie->m_Transform.translation = spawnPos;

  m_Root.AddChild(zombie);
  m_ActiveZombies.push_back({zombie, row});
}

int App::PickSpawnRowForWaveSpawn() {
  if (m_UseStandRowForNextSpawn && m_BasicZombieStandReady) {
    const float normalizedY = (m_BasicZombieStandYPercent - kGridMinYPercent) /
                              (kGridMaxYPercent - kGridMinYPercent);
    const int standRow =
        glm::clamp(static_cast<int>(normalizedY * kGridRows), 0, kGridRows - 1);

    m_BasicZombieStandReady = false;
    m_UseStandRowForNextSpawn = false;
    return standRow;
  }

  std::uniform_int_distribution<int> rowDist(0, kGridRows - 1);
  return rowDist(m_Random);
}

bool App::HasAliveZombie() const {
  for (const auto &zombie : m_ActiveZombies) {
    if (zombie.object != nullptr && !zombie.object->IsDestroyed()) {
      return true;
    }
  }
  return false;
}

float App::GridRowCenterPercent(const int row) const {
  const float cellHeightPercent =
      (kGridMaxYPercent - kGridMinYPercent) / static_cast<float>(kGridRows);
  return kGridMinYPercent +
         (static_cast<float>(row) + 0.5F) * cellHeightPercent;
}

glm::vec2 App::ScreenPercentToRootLocal(const float xPercent,
                                        const float yPercent) const {
  const float pixelX = (xPercent / 100.0F) * static_cast<float>(WINDOW_WIDTH);
  const float pixelY = (yPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);

  const float cursorX = pixelX - static_cast<float>(WINDOW_WIDTH) * 0.5F;
  const float cursorY = static_cast<float>(WINDOW_HEIGHT) * 0.5F - pixelY;

  constexpr float kCameraOffsetY = 0.05F * static_cast<float>(WINDOW_HEIGHT);
  return glm::vec2(cursorX, cursorY) -
         glm::vec2(m_CameraCurrentX, kCameraOffsetY);
}

std::vector<std::shared_ptr<Plant>> App::CollectAlivePlants() const {
  std::vector<std::shared_ptr<Plant>> plants;
  plants.reserve(kGridCellCount);

  for (const auto &sunflower : m_Sunflowers) {
    if (sunflower != nullptr && !sunflower->IsDead()) {
      plants.push_back(sunflower);
    }
  }

  for (const auto &peashooter : m_Peashooters) {
    if (peashooter != nullptr && !peashooter->IsDead()) {
      plants.push_back(peashooter);
    }
  }

  for (const auto &nut : m_Nuts) {
    if (nut != nullptr && !nut->IsDead()) {
      plants.push_back(nut);
    }
  }

  for (const auto &cherryBomb : m_CherryBombs) {
    if (cherryBomb != nullptr && !cherryBomb->IsDead()) {
      plants.push_back(cherryBomb);
    }
  }

  return plants;
}

void App::SetupPlantCards() {
  if (!PrepareSunflowerFrames() || !PreparePeashooterFrames() ||
      !PrepareNutFrames() || !PrepareCherryBombFrames()) {
    return;
  }

  m_PlantCards = {
      {
          PlantCardSelection::SUNFLOWER,
          kSunflowerCost,
          m_SunflowerCard,
          m_SunflowerCardGrayMask,
          "Resources/ui/cards/sunflower.png",
          "Resources/ui/cards/generated/sunflower_gray.png",
      },
      {
          PlantCardSelection::PEASHOOTER,
          kPeashooterCost,
          m_PeashooterCard,
          m_PeashooterCardGrayMask,
          "Resources/ui/cards/peashooter.png",
          "Resources/ui/cards/generated/peashooter_gray.png",
      },
      {
          PlantCardSelection::NUT,
          kNutCost,
          m_NutCard,
          m_NutCardGrayMask,
          "Resources/ui/cards/wall-nut.png",
          "Resources/ui/cards/generated/wall-nut_gray.png",
      },
      {
          PlantCardSelection::CHERRY_BOMB,
          kCherryBombCost,
          m_CherryBombCard,
          m_CherryBombCardGrayMask,
          "Resources/ui/cards/cherry-bomb.png",
          "Resources/ui/cards/generated/cherry-bomb_gray.png",
      },
  };

  for (const auto &card : m_PlantCards) {
    PrepareGrayCardImage(card.normalImagePath, card.disabledImagePath);
  }

  const glm::vec2 sourceSize = m_CardSlot->GetSourceSize();
  const glm::vec2 slotSize = m_CardSlot->GetScaledSize();
  if (sourceSize.x <= 0.0F || sourceSize.y <= 0.0F || slotSize.x <= 0.0F ||
      slotSize.y <= 0.0F) {
    return;
  }

  constexpr float kCardsX1 = 113.0F;
  constexpr float kCardsY1 = 10.0F;
  constexpr float kCardsY2 = 97.0F;
  const float cardRegionHeight = kCardsY2 - kCardsY1;
  const float centerY = (kCardsY1 + kCardsY2) * 0.5F;
  constexpr float kLeftPadding = 12.0F;
  constexpr float kCardGap = 14.0F;

  const float scaleY = slotSize.y / sourceSize.y;
  const float targetHeightPx = cardRegionHeight * scaleY;

  const auto setupCard = [&](const std::shared_ptr<Util::GameObject> &card,
                             const std::string &imagePath,
                             const float sourceLeftX) {
    auto image = std::make_shared<Util::Image>(imagePath);
    card->SetDrawable(image);
    card->SetZIndex(12.0F);
    card->SetVisible(false);

    const glm::vec2 cardSourceSize = image->GetSize();
    float cardWidthSourceScaled = 0.0F;
    if (cardSourceSize.y > 0.0F) {
      const float scale = targetHeightPx / cardSourceSize.y;
      card->m_Transform.scale = {scale, scale};
      cardWidthSourceScaled = (cardSourceSize.x * scale) / scaleY;
    }

    const float sourceCenterX = sourceLeftX + cardWidthSourceScaled * 0.5F;
    card->m_Transform.translation =
        CardSlotLocalFromSourceCoord(sourceCenterX, centerY);
    m_UIRoot.AddChild(card);

    return cardWidthSourceScaled;
  };

  float nextLeft = kCardsX1 + kLeftPadding;
  for (auto &card : m_PlantCards) {
    const float width = setupCard(card.normal, card.normalImagePath, nextLeft);

    auto disabledImage = std::make_shared<Util::Image>(card.disabledImagePath);
    card.disabled->SetDrawable(disabledImage);
    card.disabled->SetZIndex(12.5F);
    card.disabled->SetVisible(false);
    card.disabled->m_Transform.scale = card.normal->m_Transform.scale;
    card.disabled->m_Transform.translation =
        card.normal->m_Transform.translation;
    m_UIRoot.AddChild(card.disabled);

    nextLeft += width + kCardGap;
  }

  constexpr float kShovelShellGap = 18.0F;
  setupCard(m_ShovelShell, "Resources/ui/tools/shovel/shovel_shell.png",
            sourceSize.x + kShovelShellGap);
  m_ShovelShell->SetZIndex(12.2F);
  m_ShovelShell->SetVisible(false);

  auto shovelImage =
      std::make_shared<Util::Image>("Resources/ui/tools/shovel/shovel.png");
  m_Shovel->SetDrawable(shovelImage);
  m_Shovel->SetZIndex(12.8F);
  m_Shovel->SetVisible(false);
  if (shovelImage->GetSize().y > 0.0F) {
    const float shovelScale =
        (targetHeightPx / shovelImage->GetSize().y) * 0.7F;
    m_Shovel->m_Transform.scale = {shovelScale, shovelScale};
  }
  m_Shovel->m_Transform.translation = m_ShovelShell->m_Transform.translation;
  m_UIRoot.AddChild(m_ShovelShell);
  m_UIRoot.AddChild(m_Shovel);

  m_SelectedPlantPreview->SetZIndex(13.0F);
  m_SelectedPlantPreview->SetVisible(false);
  m_UIRoot.AddChild(m_SelectedPlantPreview);
}

bool App::TrySelectPlantCardAt(const float pixelX, const float pixelY) {
  if (m_CameraStage != CameraStage::FINISHED) {
    return false;
  }

  for (const auto &card : m_PlantCards) {
    if (!IsPixelInsideObject(card.normal, pixelX, pixelY)) {
      continue;
    }
    if (m_Sunlight < card.cost) {
      return false;
    }

    m_SelectedPlant = card.selection;
    std::shared_ptr<Util::Image> preview = nullptr;
    if (card.selection == PlantCardSelection::SUNFLOWER) {
      preview = std::make_shared<Util::Image>(m_SunflowerFramePaths.front());
    } else if (card.selection == PlantCardSelection::PEASHOOTER) {
      preview = std::make_shared<Util::Image>(m_PeashooterFramePaths.front());
    } else if (card.selection == PlantCardSelection::NUT) {
      preview = std::make_shared<Util::Image>(m_Nut1FramePaths.front());
    } else if (card.selection == PlantCardSelection::CHERRY_BOMB) {
      preview = std::make_shared<Util::Image>(m_CherryBombFramePaths.front());
    }

    if (preview == nullptr) {
      return false;
    }

    m_SelectedPlantPreview->SetDrawable(preview);
    const glm::vec2 previewSize = preview->GetSize();
    if (previewSize.y > 0.0F) {
      const float scale = ComputePlantPreviewTargetHeight() / previewSize.y;
      m_SelectedPlantPreview->m_Transform.scale = {scale, scale};
    }
    m_SelectedPlantPreview->SetVisible(true);
    UpdateSelectedPlantPreview();
    return true;
  }

  if (IsPixelInsideObject(m_ShovelShell, pixelX, pixelY) ||
      IsPixelInsideObject(m_Shovel, pixelX, pixelY)) {
    m_SelectedPlant = PlantCardSelection::SHOVEL;

    auto preview =
        std::make_shared<Util::Image>("Resources/ui/tools/shovel/shovel.png");
    m_SelectedPlantPreview->SetDrawable(preview);
    m_SelectedPlantPreview->m_Transform.scale = m_Shovel->m_Transform.scale;
    m_SelectedPlantPreview->SetVisible(true);
    UpdateSelectedPlantPreview();
    return true;
  }

  return false;
}

void App::UpdateSelectedPlantPreview() {
  if (m_SelectedPlant == PlantCardSelection::NONE) {
    m_SelectedPlantPreview->SetVisible(false);
    return;
  }

  const glm::vec2 cursor = Util::Input::GetCursorPosition();
  if (m_SelectedPlant == PlantCardSelection::SHOVEL) {
    m_SelectedPlantPreview->m_Transform.translation =
        cursor + glm::vec2(18.0F, -12.0F);
    return;
  }

  m_SelectedPlantPreview->m_Transform.translation =
      cursor + glm::vec2(34.0F, -24.0F);
}

bool App::PreparePlantPlacement(const int row, const int column, int &index,
                                glm::vec2 &localPosition) const {
  index = row * kGridColumns + column;
  if (IsCellOccupied(index)) {
    return false;
  }

  localPosition = ComputeGridCellLocalPosition(row, column);
  return true;
}

bool App::PlaceSunflowerAtGridCell(const int row, const int column) {
  if (!PrepareSunflowerFrames()) {
    return false;
  }

  if (m_Sunlight < kSunflowerCost) {
    return false;
  }

  int index = 0;
  glm::vec2 localPosition = {0.0F, 0.0F};
  if (!PreparePlantPlacement(row, column, index, localPosition)) {
    return false;
  }

  auto sunflower = std::make_shared<Sunflower>(
      m_SunflowerFramePaths,
      static_cast<std::size_t>(m_SunflowerFrameIntervalMs),
      ComputePlantTargetHeight());
  sunflower->m_Transform.translation = localPosition;

  m_Sunflowers[static_cast<std::size_t>(index)] = sunflower;
  m_Root.AddChild(sunflower);
  m_Sunlight -= kSunflowerCost;
  return true;
}

bool App::PlacePeashooterAtGridCell(const int row, const int column) {
  if (!PreparePeashooterFrames()) {
    return false;
  }

  if (m_Sunlight < kPeashooterCost) {
    return false;
  }

  int index = 0;
  glm::vec2 localPosition = {0.0F, 0.0F};
  if (!PreparePlantPlacement(row, column, index, localPosition)) {
    return false;
  }

  auto peashooter = std::make_shared<Peashooter>(
      m_PeashooterFramePaths,
      static_cast<std::size_t>(m_PeashooterFrameIntervalMs),
      ComputePlantTargetHeight());
  peashooter->m_Transform.translation = localPosition;

  m_Peashooters[static_cast<std::size_t>(index)] = peashooter;
  m_Root.AddChild(peashooter);
  m_Sunlight -= kPeashooterCost;
  return true;
}

bool App::PlaceNutAtGridCell(const int row, const int column) {
  if (!PrepareNutFrames()) {
    return false;
  }

  if (m_Sunlight < kNutCost) {
    return false;
  }

  int index = 0;
  glm::vec2 localPosition = {0.0F, 0.0F};
  if (!PreparePlantPlacement(row, column, index, localPosition)) {
    return false;
  }

  auto nut = std::make_shared<Nut>(
      m_Nut1FramePaths, static_cast<std::size_t>(m_Nut1FrameIntervalMs),
      m_Nut2FramePaths, static_cast<std::size_t>(m_Nut2FrameIntervalMs),
      m_Nut3FramePaths, static_cast<std::size_t>(m_Nut3FrameIntervalMs),
      m_Nut4FramePaths, static_cast<std::size_t>(m_Nut4FrameIntervalMs),
      ComputePlantTargetHeight());
  nut->m_Transform.translation = localPosition;

  m_Nuts[static_cast<std::size_t>(index)] = nut;
  m_Root.AddChild(nut);
  m_Sunlight -= kNutCost;
  return true;
}

bool App::PlaceCherryBombAtGridCell(const int row, const int column) {
  if (!PrepareCherryBombFrames() || !PrepareCherryBombBlowFrames()) {
    return false;
  }

  if (m_Sunlight < kCherryBombCost) {
    return false;
  }

  int index = 0;
  glm::vec2 localPosition = {0.0F, 0.0F};
  if (!PreparePlantPlacement(row, column, index, localPosition)) {
    return false;
  }

  auto cherryBomb = std::make_shared<CherryBomb>(
      m_CherryBombFramePaths,
      static_cast<std::size_t>(m_CherryBombFrameIntervalMs),
      m_CherryBombBlowFramePaths,
      static_cast<std::size_t>(m_CherryBombBlowFrameIntervalMs),
      ComputePlantTargetHeight());
  cherryBomb->m_Transform.translation = localPosition;

  m_CherryBombs[static_cast<std::size_t>(index)] = cherryBomb;
  m_Root.AddChild(cherryBomb);
  m_Sunlight -= kCherryBombCost;
  return true;
}

bool App::RemovePlantAtGridCell(const int row, const int column) {
  const int index = row * kGridColumns + column;
  bool removed = false;

  const auto removePlant = [&](auto &plant) {
    if (plant != nullptr) {
      // Mark as dead first so zombies currently targeting this plant
      // immediately leave attacking state on their next update.
      plant->TakeDamage(plant->GetHealth());
      m_Root.RemoveChild(plant);
      plant = nullptr;
      removed = true;
    }
  };

  auto &sunflower = m_Sunflowers[static_cast<std::size_t>(index)];
  removePlant(sunflower);

  auto &peashooter = m_Peashooters[static_cast<std::size_t>(index)];
  removePlant(peashooter);

  auto &nut = m_Nuts[static_cast<std::size_t>(index)];
  removePlant(nut);

  auto &cherryBomb = m_CherryBombs[static_cast<std::size_t>(index)];
  removePlant(cherryBomb);

  return removed;
}

void App::SpawnFallingSun() {
  constexpr float kSunHeightPercent = 10.0F;
  constexpr float kSpawnYPercent = 20.0F;
  constexpr float kStopMinYPercent = 52.0F;
  constexpr float kStopMaxYPercent = 78.0F;
  std::uniform_real_distribution<float> xPercentDist(8.0F, 92.0F);
  std::uniform_real_distribution<float> stopYPercentDist(kStopMinYPercent,
                                                         kStopMaxYPercent);

  const float spawnXPercent = xPercentDist(m_Random);
  const float spawnPixelX =
      (spawnXPercent / 100.0F) * static_cast<float>(WINDOW_WIDTH);
  const float spawnPixelY =
      (kSpawnYPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);

  const float localX = spawnPixelX - static_cast<float>(WINDOW_WIDTH) * 0.5F;
  const float localY = static_cast<float>(WINDOW_HEIGHT) * 0.5F - spawnPixelY;
  const float stopPixelY =
      (stopYPercentDist(m_Random) / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  const float stopLocalY =
      static_cast<float>(WINDOW_HEIGHT) * 0.5F - stopPixelY;

  const float sunHeightPx =
      (kSunHeightPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  auto sun = std::make_shared<Sun>(sunHeightPx);
  sun->m_Transform.translation = {localX, localY};

  ActiveSun activeSun;
  activeSun.object = sun;
  activeSun.falling = true;
  activeSun.stopped = false;
  activeSun.stopLocalY = stopLocalY;
  activeSun.fromSky = true;
  activeSun.expires = true;
  m_UIRoot.AddChild(sun);
  m_Suns.push_back(activeSun);
}

void App::SpawnSunFromSunflower(const std::shared_ptr<Sunflower> &sunflower) {
  constexpr float kSunHeightPercent = 10.0F;
  constexpr float kPopDistancePercent = 7.0F;
  constexpr float kCameraOffsetY = 0.05F * static_cast<float>(WINDOW_HEIGHT);

  if (sunflower == nullptr) {
    return;
  }

  const float sunHeightPx =
      (kSunHeightPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  auto sun = std::make_shared<Sun>(sunHeightPx);

  const glm::vec2 rootToScreenOffset = {m_CameraCurrentX, kCameraOffsetY};
  const glm::vec2 startPosition = sunflower->m_Transform.translation +
                                  sunflower->GetSunSpawnOffset() +
                                  rootToScreenOffset;
  const glm::vec2 targetPosition =
      sunflower->m_Transform.translation +
      sunflower->GetSunPopTargetOffset((kPopDistancePercent / 100.0F) *
                                       static_cast<float>(WINDOW_HEIGHT)) +
      rootToScreenOffset;
  sun->m_Transform.translation = startPosition;

  ActiveSun activeSun;
  activeSun.object = sun;
  activeSun.producer = sunflower;
  activeSun.falling = false;
  activeSun.expires = false;
  activeSun.rising = true;
  activeSun.riseStart = startPosition;
  activeSun.riseTarget = targetPosition;
  m_UIRoot.AddChild(sun);
  m_Suns.push_back(activeSun);
}

glm::vec2 App::CardSlotLocalFromSourceCoord(const float sourceX,
                                            const float sourceY) const {
  const glm::vec2 slotSize = m_CardSlot->GetScaledSize();
  const glm::vec2 sourceSize = m_CardSlot->GetSourceSize();
  if (slotSize.x <= 0.0F || slotSize.y <= 0.0F || sourceSize.x <= 0.0F ||
      sourceSize.y <= 0.0F) {
    return {0.0F, 0.0F};
  }

  const float scaleX = slotSize.x / sourceSize.x;
  const float scaleY = slotSize.y / sourceSize.y;

  const float leftPx = (sourceX - sourceSize.x * 0.5F) * scaleX;
  const float topPx = (sourceY - sourceSize.y * 0.5F) * scaleY;

  return {
      m_CardSlot->m_Transform.translation.x + leftPx,
      m_CardSlot->m_Transform.translation.y - topPx,
  };
}

void App::UpdateSuns(const float deltaTime) {
  m_SunSpawnCountdown -= deltaTime;
  if (m_SunSpawnCountdown <= 0.0F) {
    SpawnFallingSun();
    m_SunSpawnCountdown = 8.0F;
  }

  for (const auto &sunflower : m_Sunflowers) {
    if (sunflower == nullptr || !sunflower->ShouldProduceSun(deltaTime)) {
      continue;
    }

    SpawnSunFromSunflower(sunflower);
  }

  const float dropSpeedPx = 0.05F * static_cast<float>(WINDOW_HEIGHT);
  const glm::vec2 collectTargetLocal = CardSlotLocalFromSourceCoord(
      (17.0F + 92.0F) * 0.5F, (15.0F + 91.0F) * 0.5F);
  constexpr float kCollectMoveSeconds = 0.30F;
  constexpr float kRiseMoveSeconds = 0.35F;

  for (std::size_t i = 0; i < m_Suns.size(); ++i) {
    auto &sun = m_Suns[i];

    if (sun.collecting) {
      sun.collectElapsed += deltaTime;
      const float t =
          glm::clamp(sun.collectElapsed / kCollectMoveSeconds, 0.0F, 1.0F);
      sun.object->m_Transform.translation = {
          Lerp(sun.collectStart.x, collectTargetLocal.x, t),
          Lerp(sun.collectStart.y, collectTargetLocal.y, t),
      };
      continue;
    }

    sun.aliveSeconds += deltaTime;

    if (sun.rising) {
      sun.riseElapsed += deltaTime;
      const float t =
          glm::clamp(sun.riseElapsed / kRiseMoveSeconds, 0.0F, 1.0F);
      const float easedT = 1.0F - (1.0F - t) * (1.0F - t);
      sun.object->m_Transform.translation = {
          Lerp(sun.riseStart.x, sun.riseTarget.x, easedT),
          Lerp(sun.riseStart.y, sun.riseTarget.y, easedT),
      };
      if (t >= 1.0F) {
        sun.rising = false;
      }
      continue;
    }

    if (sun.falling) {
      sun.object->m_Transform.translation.y -= dropSpeedPx * deltaTime;

      if (sun.fromSky && !sun.stopped &&
          sun.object->m_Transform.translation.y <= sun.stopLocalY) {
        sun.object->m_Transform.translation.y = sun.stopLocalY;
        sun.falling = false;
        sun.stopped = true;
        sun.stoppedSeconds = 0.0F;
      }
    }

    if (sun.fromSky && sun.stopped) {
      sun.stoppedSeconds += deltaTime;
    }
  }

  for (std::size_t i = 0; i < m_Suns.size();) {
    const auto &sun = m_Suns[i];

    if (sun.collecting && sun.collectElapsed >= kCollectMoveSeconds) {
      if (const auto producer = sun.producer.lock(); producer != nullptr) {
        producer->OnProducedSunCollected();
      }
      RemoveSunAt(i);
      m_Sunlight += 25;
      continue;
    }

    const glm::vec2 sunSize = sun.object->GetScaledSize();
    const float centerY = static_cast<float>(WINDOW_HEIGHT) * 0.5F -
                          sun.object->m_Transform.translation.y;
    const float sunTopPixel = centerY - sunSize.y * 0.5F;
    if (!sun.collecting && sun.expires &&
        ((sun.fromSky && sun.stopped && sun.stoppedSeconds > 5.0F) ||
         (sun.falling && sunTopPixel > static_cast<float>(WINDOW_HEIGHT)))) {
      RemoveSunAt(i);
      continue;
    }

    ++i;
  }
}

void App::SetupBasicZombieStand() {
  constexpr float kStandAppearSeconds = 1.2F;

  if (m_BasicZombieStandReady || !m_CameraInitialized) {
    return;
  }
  if (m_CameraStage != CameraStage::STAGE2_RIGHT ||
      m_CameraStageElapsed < kStandAppearSeconds) {
    return;
  }
  if (m_BasicZombieStands.empty()) {
    return;
  }

  m_BasicZombieStandReady = true;
}

void App::UpdateBasicZombie(const float deltaTime) {
  if (deltaTime <= 0.0F) {
    return;
  }

  for (std::size_t i = 0; i < m_ActiveZombies.size();) {
    auto &zombie = m_ActiveZombies[i];
    if (zombie.object != nullptr) {
      zombie.object->Update(deltaTime, CollectAlivePlants());
      if (zombie.object->IsDestroyed()) {
        m_Root.RemoveChild(zombie.object);
        m_ActiveZombies.erase(m_ActiveZombies.begin() + static_cast<long>(i));
        continue;
      }
    }
    ++i;
  }

  if (!m_WaveSystemStarted) {
    return;
  }

  m_WaveElapsedSec += deltaTime;

  if (!m_WaveGroupActive && m_CurrentWaveGroupIndex < m_ZombieWavePlan.size()) {
    const ZombieWaveSpawnGroup &candidate =
        m_ZombieWavePlan[m_CurrentWaveGroupIndex];
    const bool hasReachedStart = m_WaveElapsedSec >= candidate.earliestStartSec;
    const bool clearConditionOk =
        !candidate.waitUntilClear || !HasAliveZombie();
    const bool isHugeWave =
        (candidate.phaseId == "huge_wave") || (candidate.phaseType == "huge");

    if (hasReachedStart && clearConditionOk) {
      if (isHugeWave) {
        TriggerHugeWaveBanner();
      }
      if (isHugeWave && m_HugeWaveBannerRemainingSec > 0.0F) {
        return;
      }
      m_CurrentWaveGroup = candidate;
      m_WaveGroupActive = true;
      m_WaveGroupSpawnedCount = 0;
      m_WaveGroupSpawnTimer = 0.0F;
    }
  }

  if (!m_WaveGroupActive) {
    return;
  }

  m_WaveGroupSpawnTimer -= deltaTime;
  while (m_WaveGroupSpawnedCount < m_CurrentWaveGroup.zombieCount &&
         m_WaveGroupSpawnTimer <= 0.0F) {
    const int spawnRow = PickSpawnRowForWaveSpawn();
    const std::string zombieType =
        m_CurrentWaveGroup.zombieTypes.empty()
            ? m_CurrentWaveGroup.zombieType
            : m_CurrentWaveGroup
                  .zombieTypes[static_cast<std::size_t>(
                                   m_WaveGroupSpawnedCount) %
                               m_CurrentWaveGroup.zombieTypes.size()];
    SpawnZombieAtRow(spawnRow, zombieType);
    ++m_WaveGroupSpawnedCount;

    if (m_CurrentWaveGroup.spawnIntervalSec <= 0.0F) {
      m_WaveGroupSpawnTimer = 0.0F;
    } else {
      m_WaveGroupSpawnTimer += m_CurrentWaveGroup.spawnIntervalSec;
    }
  }

  if (m_WaveGroupSpawnedCount >= m_CurrentWaveGroup.zombieCount) {
    m_WaveGroupActive = false;
    ++m_CurrentWaveGroupIndex;
  }
}

void App::SetupLawnMowers() {
  if (!PrepareLawnMowerFrames()) {
    return;
  }

  for (auto &mower : m_LawnMowers) {
    if (mower.object != nullptr) {
      m_Root.RemoveChild(mower.object);
    }
    mower = {};
  }

  const float cellWidthPercent =
      (kGridMaxXPercent - kGridMinXPercent) / static_cast<float>(kGridColumns);
  const float cellHeightPercent =
      (kGridMaxYPercent - kGridMinYPercent) / static_cast<float>(kGridRows);
  const float cellWidthPx =
      (cellWidthPercent / 100.0F) * static_cast<float>(WINDOW_WIDTH);
  constexpr float kLawnMowerVisualScale = 2.00F;
  const float targetHeightPx =
      (cellHeightPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT) * 0.20F;
  const float outsideOffsetPx = cellWidthPx * 0.70F;

  for (int row = 0; row < kGridRows; ++row) {
    auto &mower = m_LawnMowers[static_cast<std::size_t>(row)];

    auto mowerObj = std::make_shared<Util::GameObject>();
    auto mowerAnim = std::make_shared<Util::Animation>(
        m_LawnMowerFramePaths, false,
        static_cast<std::size_t>(m_LawnMowerFrameIntervalMs), true, 0);
    mowerObj->SetDrawable(mowerAnim);
    mowerObj->SetVisible(false);
    mowerObj->SetZIndex(1.1F);

    const glm::vec2 animSize = mowerAnim->GetSize();
    if (animSize.y > 0.0F) {
      const float scale = (targetHeightPx / animSize.y) * kLawnMowerVisualScale;
      mowerObj->m_Transform.scale = {scale, scale};
    }

    glm::vec2 rowPos = ComputeGridCellLocalPosition(row, 0);
    rowPos.x -= outsideOffsetPx;
    mowerObj->m_Transform.translation = rowPos;

    mower.row = row;
    mower.object = mowerObj;
    mower.animation = mowerAnim;
    mower.armed = true;
    mower.active = false;
    mower.destroyed = false;

    m_Root.AddChild(mowerObj);
  }
}

void App::UpdateLawnMowers(const float deltaTime) {
  if (deltaTime <= 0.0F || m_CameraStage != CameraStage::FINISHED) {
    return;
  }

  constexpr float kLawnMowerSpeedPxPerSec =
      0.35F * static_cast<float>(WINDOW_WIDTH);

  for (auto &mower : m_LawnMowers) {
    if (mower.object == nullptr || mower.destroyed) {
      continue;
    }

    if (mower.armed) {
      for (auto &zombie : m_ActiveZombies) {
        if (zombie.object == nullptr || zombie.object->IsDestroyed() ||
            zombie.row != mower.row) {
          continue;
        }

        if (CheckCustomAABBCollision(
                *mower.object, *zombie.object,
                glm::vec2(0.80F, 0.75F), // mower: front body region
                glm::vec2(0.50F, 0.80F), // zombie: torso region
                glm::vec2(0.0F, 0.0F), glm::vec2(0.0F, 0.0F))) {
          mower.armed = false;
          mower.active = true;
          if (mower.animation != nullptr) {
            mower.animation->Play();
          }
          zombie.object->TakeDamage(9999);
          break;
        }
      }
    }

    if (!mower.active) {
      continue;
    }

    mower.object->m_Transform.translation.x +=
        kLawnMowerSpeedPxPerSec * deltaTime;

    for (auto &zombie : m_ActiveZombies) {
      if (zombie.object == nullptr || zombie.object->IsDestroyed() ||
          zombie.row != mower.row) {
        continue;
      }

      if (CheckCustomAABBCollision(
              *mower.object, *zombie.object,
              glm::vec2(0.85F, 0.75F), // mower: sweeping body region
              glm::vec2(0.50F, 0.80F), // zombie: torso region
              glm::vec2(0.0F, 0.0F), glm::vec2(0.0F, 0.0F))) {
        zombie.object->TakeDamage(9999);
      }
    }

    const glm::vec2 mowerSize = mower.object->GetScaledSize();
    const float offscreenRightX =
        static_cast<float>(WINDOW_WIDTH) * 0.55F - m_CameraCurrentX;
    if (mower.object->m_Transform.translation.x - mowerSize.x * 0.5F >
        offscreenRightX) {
      m_Root.RemoveChild(mower.object);
      mower.object = nullptr;
      mower.animation = nullptr;
      mower.active = false;
      mower.destroyed = true;
    }
  }
}

void App::UpdateCherryBombs(const float deltaTime) {
  if (deltaTime <= 0.0F) {
    return;
  }

  for (int index = 0; index < kGridCellCount; ++index) {
    auto &cherryBomb = m_CherryBombs[static_cast<std::size_t>(index)];
    if (cherryBomb == nullptr || cherryBomb->IsDead()) {
      continue;
    }

    if (!cherryBomb->UpdateAndCheckExplode(deltaTime)) {
      continue;
    }

    const int centerRow = index / kGridColumns;
    const int centerColumn = index % kGridColumns;

    for (auto &zombie : m_ActiveZombies) {
      if (zombie.object == nullptr || zombie.object->IsDestroyed()) {
        continue;
      }

      const float zombiePixelX = zombie.object->m_Transform.translation.x +
                                 m_CameraCurrentX +
                                 static_cast<float>(WINDOW_WIDTH) * 0.5F;
      const float zombieXPercent =
          (zombiePixelX / static_cast<float>(WINDOW_WIDTH)) * 100.0F;
      const float normalizedX = (zombieXPercent - kGridMinXPercent) /
                                (kGridMaxXPercent - kGridMinXPercent);
      const int zombieColumn = glm::clamp(
          static_cast<int>(normalizedX * kGridColumns), 0, kGridColumns - 1);

      if (glm::abs(zombie.row - centerRow) <= 1 &&
          glm::abs(zombieColumn - centerColumn) <= 1) {
        zombie.object->TakeDamage(9999, true);
      }
    }
  }
}

bool App::HasAliveZombieInRow(const int row, const float shooterX) const {
  for (const auto &zombie : m_ActiveZombies) {
    if (zombie.object == nullptr || zombie.object->IsDestroyed()) {
      continue;
    }
    if (zombie.row != row) {
      continue;
    }
    if (zombie.object->m_Transform.translation.x > shooterX) {
      return true;
    }
  }
  return false;
}

void App::SpawnPeaFromPeashooter(
    const std::shared_ptr<Peashooter> &peashooter) {
  if (peashooter == nullptr) {
    return;
  }

  auto pea = std::make_shared<Util::GameObject>();
  auto peaImage = std::make_shared<Util::Image>(
      "Resources/gameplay/plants/peashooter_bullet/pea.png");
  pea->SetDrawable(peaImage);
  pea->SetZIndex(1.2F);

  const float targetHeight = ComputePeaTargetHeight();
  const glm::vec2 peaNaturalSize = peaImage->GetSize();
  if (peaNaturalSize.y > 0.0F) {
    const float scale = targetHeight / peaNaturalSize.y;
    pea->m_Transform.scale = {scale, scale};
  }

  const glm::vec2 shooterSize = peashooter->GetScaledSize();
  pea->m_Transform.translation = peashooter->m_Transform.translation;
  pea->m_Transform.translation.x += shooterSize.x * 0.28F;
  const float cellHeightPercent =
      (kGridMaxYPercent - kGridMinYPercent) / static_cast<float>(kGridRows);
  const float upwardOffsetPx =
      (cellHeightPercent * 0.05F / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  pea->m_Transform.translation.y += upwardOffsetPx;

  ActivePea activePea;
  activePea.object = pea;
  m_Root.AddChild(pea);
  m_Peas.push_back(activePea);
}

void App::UpdatePeashooterCombat(const float deltaTime) {
  if (deltaTime <= 0.0F || !PreparePeashooterAttackFrames()) {
    return;
  }

  constexpr float kShootIntervalSec = 1.0F;
  for (int index = 0; index < kGridCellCount; ++index) {
    auto &peashooter = m_Peashooters[static_cast<std::size_t>(index)];
    if (peashooter == nullptr || peashooter->IsDead()) {
      continue;
    }

    const int row = index / kGridColumns;
    const bool hasZombieInRow =
        HasAliveZombieInRow(row, peashooter->m_Transform.translation.x);
    auto &cooldown =
        m_PeashooterAttackCooldowns[static_cast<std::size_t>(index)];

    if (peashooter->IsAttacking()) {
      if (peashooter->UpdateAttackStateAndCheckShoot()) {
        SpawnPeaFromPeashooter(peashooter);
      }
      continue;
    }

    if (!hasZombieInRow) {
      cooldown = kShootIntervalSec;
      continue;
    }

    cooldown -= deltaTime;
    if (cooldown <= 0.0F &&
        peashooter->StartAttack(
            m_PeashooterAttackFramePaths,
            static_cast<std::size_t>(m_PeashooterAttackFrameIntervalMs))) {
      cooldown = kShootIntervalSec;
    }
  }

  constexpr float kPeaSpeedPxPerSec = 0.45F * static_cast<float>(WINDOW_WIDTH);
  constexpr std::size_t kHitFrameIntervalMs = 50;
  for (std::size_t i = 0; i < m_Peas.size();) {
    auto &pea = m_Peas[i];
    if (!pea.hitting) {
      pea.object->m_Transform.translation.x += kPeaSpeedPxPerSec * deltaTime;

      const bool outOfRight =
          pea.object->m_Transform.translation.x >
          (static_cast<float>(WINDOW_WIDTH) * 0.6F - m_CameraCurrentX);
      if (outOfRight) {
        m_Root.RemoveChild(pea.object);
        m_Peas.erase(m_Peas.begin() + static_cast<long>(i));
        continue;
      }

      ActiveZombie *hitZombie = nullptr;
      for (auto &zombie : m_ActiveZombies) {
        if (zombie.object == nullptr || zombie.object->IsDestroyed()) {
          continue;
        }

        if (CheckCustomAABBCollision(
                *pea.object, *zombie.object,
                glm::vec2(0.80F, 0.75F), // pea: ignore transparent border
                glm::vec2(0.48F, 0.80F), // zombie: focus torso region
                glm::vec2(0.0F, 0.0F),
                glm::vec2(-zombie.object->GetScaledSize().x * 0.10F, 0.0F))) {
          hitZombie = &zombie;
          break;
        }
      }

      if (hitZombie != nullptr) {
        hitZombie->object->TakeDamage(20);

        const glm::vec2 zombieSize = hitZombie->object->GetScaledSize();
        pea.object->m_Transform.translation.x =
            hitZombie->object->m_Transform.translation.x - zombieSize.x * 0.20F;

        auto hitAnim = std::make_shared<Util::Animation>(
            m_PeaHitFramePaths, true, kHitFrameIntervalMs, false, 0);
        pea.object->SetDrawable(hitAnim);

        const glm::vec2 peaSize = pea.object->GetScaledSize();
        const glm::vec2 hitSize = hitAnim->GetSize();
        if (hitSize.y > 0.0F && peaSize.y > 0.0F) {
          const float scale = peaSize.y / hitSize.y;
          pea.object->m_Transform.scale = {scale, scale};
        }

        pea.hitting = true;
        pea.hitAnimation = hitAnim;
      }
      ++i;
      continue;
    }

    if (pea.hitAnimation != nullptr &&
        pea.hitAnimation->GetState() == Util::Animation::State::ENDED) {
      m_Root.RemoveChild(pea.object);
      m_Peas.erase(m_Peas.begin() + static_cast<long>(i));
      continue;
    }

    ++i;
  }
}

bool App::TryCollectSunAt(const float pixelX, const float pixelY) {
  for (std::size_t i = 0; i < m_Suns.size(); ++i) {
    auto &sun = m_Suns[i];
    const glm::vec2 center = {
        sun.object->m_Transform.translation.x +
            static_cast<float>(WINDOW_WIDTH) * 0.5F,
        static_cast<float>(WINDOW_HEIGHT) * 0.5F -
            sun.object->m_Transform.translation.y,
    };
    const glm::vec2 size = sun.object->GetScaledSize();
    const float halfWidth = size.x * 0.5F;
    const float halfHeight = size.y * 0.5F;

    const bool hit =
        (pixelX >= center.x - halfWidth) && (pixelX <= center.x + halfWidth) &&
        (pixelY >= center.y - halfHeight) && (pixelY <= center.y + halfHeight);
    if (!hit) {
      continue;
    }

    if (sun.collecting) {
      return true;
    }

    sun.collecting = true;
    sun.collectElapsed = 0.0F;
    sun.collectStart = sun.object->m_Transform.translation;
    sun.rising = false;
    sun.falling = false;
    return true;
  }

  return false;
}

void App::RemoveDeadPlants() {
  for (auto &sunflower : m_Sunflowers) {
    if (sunflower != nullptr && sunflower->IsDead()) {
      m_Root.RemoveChild(sunflower);
      sunflower = nullptr;
    }
  }

  for (auto &peashooter : m_Peashooters) {
    if (peashooter != nullptr && peashooter->IsDead()) {
      m_Root.RemoveChild(peashooter);
      peashooter = nullptr;
    }
  }

  for (auto &nut : m_Nuts) {
    if (nut != nullptr && nut->IsDead()) {
      m_Root.RemoveChild(nut);
      nut = nullptr;
    }
  }

  for (auto &cherryBomb : m_CherryBombs) {
    if (cherryBomb != nullptr && cherryBomb->IsDead()) {
      m_Root.RemoveChild(cherryBomb);
      cherryBomb = nullptr;
    }
  }
}

void App::UpdatePlantCardUIState() {
  if (m_CameraStage != CameraStage::FINISHED) {
    return;
  }

  for (const auto &card : m_PlantCards) {
    card.normal->SetVisible(true);
    card.disabled->SetVisible(m_Sunlight < card.cost);
  }
  m_ShovelShell->SetVisible(true);
  m_Shovel->SetVisible(m_SelectedPlant != PlantCardSelection::SHOVEL);
}

void App::ClearSelectedPlantTool() {
  m_SelectedPlant = PlantCardSelection::NONE;
  m_SelectedPlantPreview->SetVisible(false);
}

void App::HandleGridClick(const float xPercent, const float yPercent,
                          const bool collectedSun, const bool selectedCard) {
  const bool insideGrid =
      (xPercent >= kGridMinXPercent && xPercent <= kGridMaxXPercent) &&
      (yPercent >= kGridMinYPercent && yPercent <= kGridMaxYPercent);

  if (!collectedSun && !selectedCard && insideGrid &&
      m_SelectedPlant != PlantCardSelection::NONE) {
    const float normalizedX =
        (xPercent - kGridMinXPercent) / (kGridMaxXPercent - kGridMinXPercent);
    const float normalizedY =
        (yPercent - kGridMinYPercent) / (kGridMaxYPercent - kGridMinYPercent);

    const int column = glm::clamp(static_cast<int>(normalizedX * kGridColumns),
                                  0, kGridColumns - 1);
    const int row =
        glm::clamp(static_cast<int>(normalizedY * kGridRows), 0, kGridRows - 1);

    m_LastHitColumn = column + 1;
    m_LastHitRow = row + 1;
    m_HasGridHit = true;

    bool placed = false;
    if (m_SelectedPlant == PlantCardSelection::SUNFLOWER) {
      placed = PlaceSunflowerAtGridCell(row, column);
    } else if (m_SelectedPlant == PlantCardSelection::PEASHOOTER) {
      placed = PlacePeashooterAtGridCell(row, column);
    } else if (m_SelectedPlant == PlantCardSelection::NUT) {
      placed = PlaceNutAtGridCell(row, column);
    } else if (m_SelectedPlant == PlantCardSelection::CHERRY_BOMB) {
      placed = PlaceCherryBombAtGridCell(row, column);
    } else if (m_SelectedPlant == PlantCardSelection::SHOVEL) {
      placed = RemovePlantAtGridCell(row, column);
    }

    if (placed && m_SelectedPlant != PlantCardSelection::SHOVEL) {
      ClearSelectedPlantTool();
    }
    return;
  }

  m_HasGridHit = false;
}

void App::RemoveSunAt(const std::size_t index) {
  m_UIRoot.RemoveChild(m_Suns[index].object);
  m_Suns.erase(m_Suns.begin() + static_cast<long>(index));
}

void App::DrawSunlightCounter() const {
  if (m_CameraStage != CameraStage::FINISHED) {
    return;
  }

  const glm::vec2 slotSize = m_CardSlot->GetScaledSize();
  const glm::vec2 sourceSize = m_CardSlot->GetSourceSize();
  if (slotSize.x <= 0.0F || slotSize.y <= 0.0F || sourceSize.x <= 0.0F ||
      sourceSize.y <= 0.0F) {
    return;
  }

  const float centerX = m_CardSlot->m_Transform.translation.x +
                        static_cast<float>(WINDOW_WIDTH) * 0.5F;
  const float centerY = static_cast<float>(WINDOW_HEIGHT) * 0.5F -
                        m_CardSlot->m_Transform.translation.y;
  const glm::vec2 topLeft = {centerX - slotSize.x * 0.5F,
                             centerY - slotSize.y * 0.5F};

  const float scaleX = slotSize.x / sourceSize.x;
  const float scaleY = slotSize.y / sourceSize.y;

  constexpr float kCountX1 = 24.0F;
  constexpr float kCountY1 = 102.0F;
  constexpr float kCountX2 = 83.0F;
  constexpr float kCountY2 = 125.0F;

  const ImVec2 rectMin(topLeft.x + kCountX1 * scaleX,
                       topLeft.y + kCountY1 * scaleY);
  const ImVec2 rectMax(topLeft.x + kCountX2 * scaleX,
                       topLeft.y + kCountY2 * scaleY);

  const std::string sunlightText = std::to_string(m_Sunlight);
  const ImVec2 textSize = ImGui::CalcTextSize(sunlightText.c_str());
  const ImVec2 textPos(rectMin.x + (rectMax.x - rectMin.x - textSize.x) * 0.5F,
                       rectMin.y + (rectMax.y - rectMin.y - textSize.y) * 0.5F);

  ImDrawList *drawList = ImGui::GetForegroundDrawList();
  drawList->AddText(textPos, IM_COL32(28, 28, 20, 255), sunlightText.c_str());
}

void App::SetupBannerObject(const std::shared_ptr<Util::GameObject> &banner,
                            const std::string &imagePath, const float zIndex,
                            const bool fullScreen) const {
  auto image = std::make_shared<Util::Image>(imagePath);
  banner->SetDrawable(image);
  banner->SetZIndex(zIndex);
  banner->SetVisible(false);

  const glm::vec2 imageSize = image->GetSize();
  if (imageSize.x <= 0.0F || imageSize.y <= 0.0F) {
    return;
  }

  const float scaleX = static_cast<float>(WINDOW_WIDTH) / imageSize.x;
  if (fullScreen) {
    const float scaleY = static_cast<float>(WINDOW_HEIGHT) / imageSize.y;
    banner->m_Transform.scale = {scaleX, scaleY};
    banner->m_Transform.translation = {0.0F, 0.0F};
    return;
  }

  const float uniformScale = scaleX;
  banner->m_Transform.scale = {uniformScale, uniformScale};
  banner->m_Transform.translation = {0.0F, 0.0F};
}

void App::TriggerHugeWaveBanner() {
  if (m_HasShownHugeWaveBanner) {
    return;
  }

  m_HasShownHugeWaveBanner = true;
  m_HugeWaveBannerRemainingSec = 3.0F;
  m_HugeWaveBanner->SetVisible(true);
}

void App::TriggerGameOverBanner() {
  if (m_GameOverBannerRemainingSec > 0.0F) {
    return;
  }

  m_GameOverBannerRemainingSec = 3.0F;
  m_GameOverBanner->SetVisible(true);
}

void App::UpdateBannerState(const float deltaTime) {
  if (m_HugeWaveBannerRemainingSec > 0.0F) {
    m_HugeWaveBannerRemainingSec =
        glm::max(0.0F, m_HugeWaveBannerRemainingSec - deltaTime);
    m_HugeWaveBanner->SetVisible(m_HugeWaveBannerRemainingSec > 0.0F);
  }

  if (m_GameOverBannerRemainingSec > 0.0F) {
    m_GameOverBannerRemainingSec =
        glm::max(0.0F, m_GameOverBannerRemainingSec - deltaTime);
    m_GameOverBanner->SetVisible(m_GameOverBannerRemainingSec > 0.0F);
  }
}

void App::Start() {
  m_Map->SetDrawable(
      std::make_shared<Util::Image>("Resources/scenes/maps/main_map.png"));
  m_Map->SetZIndex(0.0F);
  m_Map->m_Transform.translation = {0.0F, 0.0F};

  const glm::vec2 mapSize = m_Map->GetScaledSize();
  constexpr float kZoom = 1.3F;
  m_Map->m_Transform.scale = {
      (static_cast<float>(WINDOW_WIDTH) / mapSize.x) * kZoom,
      (static_cast<float>(WINDOW_HEIGHT) / mapSize.y) * kZoom,
  };

  m_MapScaledWidth = mapSize.x * m_Map->m_Transform.scale.x;
  m_Root.AddChild(m_Map);

  // Card slot UI - fixed to screen, shown only after camera settles
  {
    m_CardSlot->SetVisible(false);

    constexpr float kTopPercent = 0.9F;
    constexpr float kBottomPercent = 14.0F;
    const float targetHeightPx = ((kBottomPercent - kTopPercent) / 100.0F) *
                                 static_cast<float>(WINDOW_HEIGHT);
    const glm::vec2 naturalSize = m_CardSlot->GetSourceSize();
    if (naturalSize.y > 0.0F) {
      const float slotScale = targetHeightPx / naturalSize.y;
      m_CardSlot->m_Transform.scale = {slotScale, slotScale};
      const float scaledWidth = naturalSize.x * slotScale;

      // Left edge at x = 21 %
      const float leftEdgePx = 0.21F * static_cast<float>(WINDOW_WIDTH);
      const float centerPtsdX = leftEdgePx + scaledWidth * 0.5F -
                                static_cast<float>(WINDOW_WIDTH) * 0.5F;

      // Vertical centre between 0.9 % and 12 % from top
      const float topPx =
          (kTopPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
      const float bottomPx =
          (kBottomPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
      const float centerPtsdY =
          static_cast<float>(WINDOW_HEIGHT) * 0.5F - (topPx + bottomPx) * 0.5F;

      m_CardSlot->m_Transform.translation = {centerPtsdX, centerPtsdY};
    }
    m_UIRoot.AddChild(m_CardSlot);
  }

  SetupPlantCards();
  PrepareCherryBombBlowFrames();
  PreparePeashooterAttackFrames();
  PrepareBasicZombieFrames();
  PrepareConeheadZombieFrames();
  PreparePolevaultingZombieFrames();
  PrepareLawnMowerFrames();

  SetupBannerObject(m_HugeWaveBanner, "Resources/ui/banners/huge_wave.png",
                    99.0F, false);
  SetupBannerObject(m_GameOverBanner, "Resources/ui/banners/game_over.png",
                    100.0F, true);
  m_UIRoot.AddChild(m_HugeWaveBanner);
  m_UIRoot.AddChild(m_GameOverBanner);

  m_PeashooterAttackCooldowns.fill(1.0F);

  m_CameraStage = CameraStage::STAGE1_HOME;
  m_CameraStageElapsed = 0.0F;
  m_CameraInitialized = false;

  m_BasicZombieStandReady = false;
  m_ActiveZombies.clear();

  m_CurrentState = State::UPDATE;
  // Initialize level manager
  m_LevelManager = std::make_shared<LevelManager>();
  m_MenuScene = std::make_shared<MenuScene>(m_LevelManager);

  m_CurrentState = State::MENU;
}

void App::UpdateCamera(const float deltaTime) {
  constexpr float kHouseHoldSeconds = 3.0F;
  constexpr float kStage2MoveSeconds = 2.0F;
  constexpr float kRoadHoldSeconds = 3.0F;
  constexpr float kPanToCenterSeconds = 1.5F;

  const float halfVisibleWidth = static_cast<float>(WINDOW_WIDTH) * 0.5F;
  const float halfMapWidth = m_MapScaledWidth * 0.5F;
  const float panLimit = glm::max(0.0F, halfMapWidth - halfVisibleWidth);

  const float leftCameraX = panLimit;
  const float rightCameraX = -panLimit;
  const float centerCameraX =
      glm::clamp(0.10F * m_MapScaledWidth, -panLimit, panLimit);
  constexpr float kCameraOffsetY = 0.05F * static_cast<float>(WINDOW_HEIGHT);

  if (!m_CameraInitialized) {
    m_CameraCurrentX = leftCameraX;
    m_Root.SetTranslation({m_CameraCurrentX, kCameraOffsetY});
    m_CameraInitialized = true;
  }

  m_CameraStageElapsed += deltaTime;

  switch (m_CameraStage) {
  case CameraStage::STAGE1_HOME:
    m_CameraCurrentX = leftCameraX;
    if (m_CameraStageElapsed >= kHouseHoldSeconds) {
      m_CameraStage = CameraStage::STAGE2_RIGHT;
      m_CameraStageElapsed = 0.0F;
      m_CameraFromX = leftCameraX;
      m_CameraToX = rightCameraX;
    }
    break;

  case CameraStage::STAGE2_RIGHT: { // NOLINT
    if (m_CameraStageElapsed < kStage2MoveSeconds) {
      const float t = m_CameraStageElapsed / kStage2MoveSeconds;
      m_CameraCurrentX = Lerp(m_CameraFromX, m_CameraToX, t);
      break;
    }

    m_CameraCurrentX = rightCameraX;
    if (m_CameraStageElapsed >= (kStage2MoveSeconds + kRoadHoldSeconds)) {
      m_CameraCurrentX = rightCameraX;
      m_CameraStage = CameraStage::STAGE3_CENTER;
      m_CameraStageElapsed = 0.0F;
      m_CameraFromX = rightCameraX;
      m_CameraToX = centerCameraX;
    }

    break;
  }

  case CameraStage::STAGE3_CENTER: {
    const float t = m_CameraStageElapsed / kPanToCenterSeconds;
    m_CameraCurrentX = Lerp(m_CameraFromX, m_CameraToX, t);
    if (m_CameraStageElapsed >= kPanToCenterSeconds) {
      m_CameraCurrentX = centerCameraX;
      m_CameraStage = CameraStage::FINISHED;
      m_CameraStageElapsed = 0.0F;
      ClearBasicZombieStandPreview();
      SetupLawnMowers();
      m_SunSystemStarted = true;
      m_WaveSystemStarted = true;
      m_WaveElapsedSec = 0.0F;
      m_SunSpawnCountdown = 6.0F;
    }
    break;
  }

  case CameraStage::FINISHED:
    m_CameraCurrentX = centerCameraX;
    m_CardSlot->SetVisible(true);
    m_SunflowerCard->SetVisible(true);
    m_PeashooterCard->SetVisible(true);
    m_ShovelShell->SetVisible(true);
    m_Shovel->SetVisible(m_SelectedPlant != PlantCardSelection::SHOVEL);
    for (auto &mower : m_LawnMowers) {
      if (mower.object != nullptr && !mower.destroyed) {
        mower.object->SetVisible(true);
      }
    }
    break;
  }
  m_Root.SetTranslation({m_CameraCurrentX, kCameraOffsetY});
}

void App::Update() {
  // Handle state machine
  switch (m_CurrentState) {
  case State::START:
    Start();
    m_CurrentState = State::MENU;
    break;

  case State::MENU:
    m_MenuScene->Render(Util::Time::GetDeltaTimeMs() / 1000.0F);
    m_SelectedLevelId =
        m_MenuScene->Update(Util::Time::GetDeltaTimeMs() / 1000.0F);

    if (m_SelectedLevelId > 0) {
      // User selected a level
      m_LevelManager->LoadLevel(m_SelectedLevelId);
      m_CurrentState = State::GAME_LOADING;
    }
    break;

  case State::GAME_LOADING:
    // Initialize level resources
    InitializeLevel();
    m_CurrentState = State::PLAYING;
    break;

  case State::PLAYING:
    UpdateGameplay(Util::Time::GetDeltaTimeMs() / 1000.0F);

    // Check win/lose conditions
    if (!HasAliveZombie() && m_WaveSystemStarted &&
        m_CurrentWaveGroupIndex >= m_ZombieWavePlan.size()) {
      m_LevelManager->CompleteLevelSuccess();
      m_CurrentState = State::LEVEL_COMPLETE;
    }
    break;

  case State::LEVEL_COMPLETE: {
    UpdateGameplay(Util::Time::GetDeltaTimeMs() / 1000.0F);

    // Show level complete screen
    ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH * 0.25F, WINDOW_HEIGHT * 0.3F),
                            ImGuiCond_Always);
    ImGui::Begin("Level Complete!", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Congratulations! Level %d completed!", m_SelectedLevelId);
    const bool canProgress = m_LevelManager->CanProgressToNextLevel();
    if (canProgress && ImGui::Button("Next Level")) {
      m_SelectedLevelId = m_LevelManager->GetNextLevelId();
      m_LevelManager->LoadLevel(m_SelectedLevelId);
      m_CurrentState = State::GAME_LOADING;
    }
    if (!canProgress) {
      ImGui::TextDisabled("Final level reached");
    }
    if (ImGui::Button("Back to Menu")) {
      m_MenuScene->Reset();
      m_CurrentState = State::MENU;
    }
    ImGui::End();
    break;
  }

  case State::LEVEL_FAILED:
    UpdateGameplay(Util::Time::GetDeltaTimeMs() / 1000.0F);

    if (m_GameOverBannerRemainingSec > 0.0F) {
      break;
    }

    // Show level failed screen
    ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH * 0.25F, WINDOW_HEIGHT * 0.3F),
                            ImGuiCond_Always);
    ImGui::Begin("Level Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Level %d failed. Zombies reached your house!",
                m_SelectedLevelId);
    if (ImGui::Button("Retry")) {
      m_LevelManager->LoadLevel(m_SelectedLevelId);
      m_CurrentState = State::GAME_LOADING;
    }
    if (ImGui::Button("Back to Menu")) {
      m_MenuScene->Reset();
      m_CurrentState = State::MENU;
    }
    ImGui::End();
    break;

  case State::END:
    // Handled by main loop
    break;

  default:
    break;
  }

  // Handle exit
  if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
    m_CurrentState = State::END;
  }
}

void App::InitializeLevel() {

  ResetLevelRuntimeState();

  m_Map->SetDrawable(
      std::make_shared<Util::Image>("Resources/scenes/maps/main_map.png"));
  m_Map->SetZIndex(0.0F);
  m_Map->m_Transform.translation = {0.0F, 0.0F};
  m_Map->m_Transform.scale = {1.0F, 1.0F};

  const glm::vec2 mapSize = m_Map->GetScaledSize();
  constexpr float kZoom = 1.3F;
  m_Map->m_Transform.scale = {
      (static_cast<float>(WINDOW_WIDTH) / mapSize.x) * kZoom,
      (static_cast<float>(WINDOW_HEIGHT) / mapSize.y) * kZoom,
  };

  m_MapScaledWidth = mapSize.x * m_Map->m_Transform.scale.x;
  m_Root.AddChild(m_Map);

  // Card slot UI - fixed to screen, shown only after camera settles
  {
    m_CardSlot->SetVisible(false);

    constexpr float kTopPercent = 0.9F;
    constexpr float kBottomPercent = 14.0F;
    const float targetHeightPx = ((kBottomPercent - kTopPercent) / 100.0F) *
                                 static_cast<float>(WINDOW_HEIGHT);
    const glm::vec2 naturalSize = m_CardSlot->GetSourceSize();
    if (naturalSize.y > 0.0F) {
      const float slotScale = targetHeightPx / naturalSize.y;
      m_CardSlot->m_Transform.scale = {slotScale, slotScale};
      const float scaledWidth = naturalSize.x * slotScale;

      // Left edge at x = 21 %
      const float leftEdgePx = 0.21F * static_cast<float>(WINDOW_WIDTH);
      const float centerPtsdX = leftEdgePx + scaledWidth * 0.5F -
                                static_cast<float>(WINDOW_WIDTH) * 0.5F;

      // Vertical centre between 0.9 % and 12 % from top
      const float topPx =
          (kTopPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
      const float bottomPx =
          (kBottomPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
      const float centerPtsdY =
          static_cast<float>(WINDOW_HEIGHT) * 0.5F - (topPx + bottomPx) * 0.5F;

      m_CardSlot->m_Transform.translation = {centerPtsdX, centerPtsdY};
    }
    m_UIRoot.AddChild(m_CardSlot);
  }

  SetupPlantCards();
  PreparePeashooterAttackFrames();
  PrepareBasicZombieFrames();
  PrepareLeaderZombieFrames();
  PrepareConeheadZombieFrames();
  PrepareLawnMowerFrames();
  SetupBannerObject(m_HugeWaveBanner, "Resources/ui/banners/huge_wave.png",
                    99.0F, false);
  SetupBannerObject(m_GameOverBanner, "Resources/ui/banners/game_over.png",
                    100.0F, true);
  m_UIRoot.AddChild(m_HugeWaveBanner);
  m_UIRoot.AddChild(m_GameOverBanner);

  // Load level configuration
  const LevelConfig &levelConfig = m_LevelManager->GetCurrentLevel();
  m_Sunlight = levelConfig.initialSunAmount;

  // Build spawn plan from current level's wave configuration
  // Convert LevelConfig phases to m_LevelWaveConfig
  m_LevelWaveConfig.levelId = "level" + std::to_string(levelConfig.levelId);
  m_LevelWaveConfig.phases.clear();
  for (const auto &phase : levelConfig.phases) {
    m_LevelWaveConfig.phases.push_back(phase);
  }

  BuildZombieSpawnPlan(m_LevelWaveConfig);
  PrepareBasicZombieStandPreview();

  m_PeashooterAttackCooldowns.fill(1.0F);

  m_CameraStage = CameraStage::STAGE1_HOME;
  m_CameraStageElapsed = 0.0F;
  m_CameraInitialized = false;

  m_ActiveZombies.clear();
  m_LawnMowers.fill({});
  m_BasicZombieStandReady = false;
  m_UseStandRowForNextSpawn = true;
  m_WaveSystemStarted = false;
  m_WaveElapsedSec = 0.0F;
  m_CurrentWaveGroupIndex = 0;
  m_WaveGroupActive = false;
  m_WaveGroupSpawnedCount = 0;
  m_WaveGroupSpawnTimer = 0.0F;
  m_HasShownHugeWaveBanner = false;
  m_HugeWaveBannerRemainingSec = 0.0F;
  m_GameOverBannerRemainingSec = 0.0F;

  m_SunSystemStarted = false;
  m_SunSpawnCountdown = 0.0F;
}

void App::ResetLevelRuntimeState() {
  m_Root = Util::Renderer();
  m_UIRoot = Util::Renderer();

  m_MapScaledWidth = 0.0F;
  m_CameraCurrentX = 0.0F;
  m_CameraFromX = 0.0F;
  m_CameraToX = 0.0F;

  m_HasClickedPoint = false;
  m_HasGridHit = false;
  m_LastClickPixel = {0.0F, 0.0F};
  m_LastClickPercent = {0.0F, 0.0F};
  m_LastHitRow = 0;
  m_LastHitColumn = 0;

  m_Sunflowers.fill(nullptr);
  m_Peashooters.fill(nullptr);
  m_Nuts.fill(nullptr);
  m_Peas.clear();
  m_ActiveZombies.clear();
  m_Suns.clear();
  m_LawnMowers.fill({});

  ClearBasicZombieStandPreview();

  m_PlantCards.clear();
  m_SelectedPlant = PlantCardSelection::NONE;

  m_BasicZombieStandReady = false;
  m_UseStandRowForNextSpawn = true;
  m_BasicZombieStandYPercent = 0.0F;

  m_ZombieWavePlan.clear();
  m_CurrentWaveGroupIndex = 0;
  m_WaveGroupActive = false;
  m_WaveGroupSpawnedCount = 0;
  m_WaveGroupSpawnTimer = 0.0F;
  m_CurrentWaveGroup = {};
  m_HasShownHugeWaveBanner = false;
  m_HugeWaveBannerRemainingSec = 0.0F;
  m_GameOverBannerRemainingSec = 0.0F;

  m_HugeWaveBanner->SetVisible(false);
  m_GameOverBanner->SetVisible(false);

  m_SunSystemStarted = false;
  m_SunSpawnCountdown = 0.0F;
}

void App::UpdateGameplay(float deltaTime) {
  const float dt = deltaTime;

  UpdateBannerState(dt);

  // If level failed, just render the frozen scene without updating game logic
  if (m_CurrentState == State::LEVEL_FAILED) {
    m_Root.Update();
    m_UIRoot.Update();
    return;
  }

  UpdateCamera(dt);
  SetupBasicZombieStand();
  if (m_SunSystemStarted) {
    UpdateSuns(dt);
  }
  UpdateBasicZombie(dt);
  UpdateLawnMowers(dt);
  UpdateCherryBombs(deltaTime);
  UpdatePeashooterCombat(dt);

  RemoveDeadPlants();
  UpdatePlantCardUIState();

  // Check for game over: zombie reached left boundary
  for (const auto &zombie : m_ActiveZombies) {
    if (zombie.object == nullptr || zombie.object->IsDestroyed()) {
      continue;
    }

    const float zombiePixelX = zombie.object->m_Transform.translation.x +
                               m_CameraCurrentX +
                               static_cast<float>(WINDOW_WIDTH) * 0.5F;
    const float zombieXPercent =
        (zombiePixelX / static_cast<float>(WINDOW_WIDTH)) * 100.0F;

    if (zombieXPercent < kGridMinXPercent) {
      TriggerGameOverBanner();
      m_LevelManager->CompleteLevelFailure();
      m_CurrentState = State::LEVEL_FAILED;

      // Force one immediate render update so the game-over banner is visible
      // as soon as the fail state is entered.
      m_Root.Update();
      m_UIRoot.Update();
      DrawSunlightCounter();
      return;
    }
  }

  if (Util::Input::IsKeyPressed(Util::Keycode::MOUSE_RB)) {
    ClearSelectedPlantTool();
  }

  if (Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB)) {
    const glm::vec2 cursor = Util::Input::GetCursorPosition();
    const float pixelX = cursor.x + static_cast<float>(WINDOW_WIDTH) * 0.5F;
    const float pixelY = static_cast<float>(WINDOW_HEIGHT) * 0.5F - cursor.y;

    const float xPercent = (pixelX / static_cast<float>(WINDOW_WIDTH)) * 100.0F;
    const float yPercent =
        (pixelY / static_cast<float>(WINDOW_HEIGHT)) * 100.0F;

    m_HasClickedPoint = true;
    m_LastClickPixel = {pixelX, pixelY};
    m_LastClickPercent = {glm::clamp(xPercent, 0.0F, 100.0F),
                          glm::clamp(yPercent, 0.0F, 100.0F)};

    const bool collectedSun = TryCollectSunAt(pixelX, pixelY);
    const bool selectedCard = TrySelectPlantCardAt(pixelX, pixelY);

    HandleGridClick(xPercent, yPercent, collectedSun, selectedCard);
  }

  UpdateSelectedPlantPreview();

  m_Root.Update();
  m_UIRoot.Update();
  DrawSunlightCounter();

  if (m_HasClickedPoint) {
    ImGui::SetNextWindowPos(ImVec2(16.0F, 16.0F), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.70F);
    ImGui::Begin(
        "Click Debug", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::Text("pixel: (%.1f, %.1f)", m_LastClickPixel.x, m_LastClickPixel.y);
    ImGui::Text("percent: x=%.2f%%, y=%.2f%%", m_LastClickPercent.x,
                m_LastClickPercent.y);
    ImGui::End();

    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    const ImVec2 clickPos(m_LastClickPixel.x, m_LastClickPixel.y);
    drawList->AddCircleFilled(clickPos, 4.0F, IM_COL32(255, 70, 70, 255));
    drawList->AddLine(ImVec2(clickPos.x - 10.0F, clickPos.y),
                      ImVec2(clickPos.x + 10.0F, clickPos.y),
                      IM_COL32(255, 70, 70, 255), 2.0F);
    drawList->AddLine(ImVec2(clickPos.x, clickPos.y - 10.0F),
                      ImVec2(clickPos.x, clickPos.y + 10.0F),
                      IM_COL32(255, 70, 70, 255), 2.0F);
  }
}

void App::End() { // NOLINT(this method will mutate members in the future)
}
