#include "LevelManager.hpp"

#include <spdlog/spdlog.h>

LevelManager::LevelManager()
    : m_GameState(GameSceneState::MENU), m_CurrentLevelId(1) {
  LoadLevel(1);
}

void LevelManager::LoadLevel(int levelId) {
  if (levelId < 1 || levelId > 10) {
    spdlog::warn("Invalid level ID: {}, clamping to valid range", levelId);
    levelId = std::max(1, std::min(10, levelId));
  }

  m_CurrentLevelId = levelId;
  m_LevelCompleted = false;
  m_LevelFailed = false;
  m_GameState = GameSceneState::LOADING;

  // Build config file path
  std::string configPath =
      "Resources/levels/level" + std::to_string(levelId) + ".json";

  // Load configuration
  m_CurrentLevelConfig = LevelConfigLoader::LoadFromFile(configPath);

  spdlog::info("Level manager loaded level {} with {} phases", levelId,
               m_CurrentLevelConfig.phases.size());
}

bool LevelManager::IsLevelComplete() const { return m_LevelCompleted; }

bool LevelManager::IsLevelFailed() const { return m_LevelFailed; }

void LevelManager::CompleteLevelSuccess() {
  m_LevelCompleted = true;
  m_GameState = GameSceneState::LEVELCOMPLETE;
  spdlog::info("Level {} completed successfully!", m_CurrentLevelId);
}

void LevelManager::CompleteLevelFailure() {
  m_LevelFailed = true;
  m_GameState = GameSceneState::LEVELYFAILED;
  spdlog::warn("Level {} failed!", m_CurrentLevelId);
}

int LevelManager::GetNextLevelId() const {
  int nextId = m_CurrentLevelId + 1;
  if (nextId > 10) {
    return 10; // Cap at level 10
  }
  return nextId;
}

bool LevelManager::CanProgressToNextLevel() const {
  return m_LevelCompleted && m_CurrentLevelId < 10;
}
