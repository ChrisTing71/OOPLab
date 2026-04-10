#ifndef LEVELMANAGER_HPP
#define LEVELMANAGER_HPP

#include "LevelConfig.hpp"
#include <memory>
#include <vector>

/**
 * Game scene states
 */
enum class GameSceneState {
  MENU,          // Main menu - level selection
  LOADING,       // Loading level resources
  PLAYING,       // Actively playing the level
  PAUSED,        // Game paused
  LEVELCOMPLETE, // Level successfully completed
  LEVELYFAILED,  // Level failed (zombies reached end)
  GAMEOVER,      // Game over - failed too many levels
};

/**
 * Manager for handling level progression and state
 */
class LevelManager {
public:
  LevelManager();
  ~LevelManager() = default;

  /**
   * Initialize with a specific level ID
   * Loads the level configuration file
   * @param levelId Level ID (1-10)
   */
  void LoadLevel(int levelId);

  /**
   * Get the currently loaded level configuration
   */
  const LevelConfig &GetCurrentLevel() const { return m_CurrentLevelConfig; }

  /**
   * Get current level ID
   */
  int GetCurrentLevelId() const { return m_CurrentLevelConfig.levelId; }

  /**
   * Get current game state
   */
  GameSceneState GetGameState() const { return m_GameState; }

  /**
   * Set game state
   */
  void SetGameState(GameSceneState newState) { m_GameState = newState; }

  /**
   * Check if level is completed (all waves finished without failure)
   */
  bool IsLevelComplete() const;

  /**
   * Check if level is failed (zombies reached end)
   */
  bool IsLevelFailed() const;

  /**
   * Mark level as complete (called by App when all waves finished)
   */
  void CompleteLevelSuccess();

  /**
   * Mark level as failed (called by App when zombies reach end)
   */
  void CompleteLevelFailure();

  /**
   * Get next available level ID
   */
  int GetNextLevelId() const;

  /**
   * Check if we can unlock next level
   */
  bool CanProgressToNextLevel() const;

private:
  LevelConfig m_CurrentLevelConfig;
  GameSceneState m_GameState = GameSceneState::MENU;
  bool m_LevelCompleted = false;
  bool m_LevelFailed = false;
  int m_CurrentLevelId = 1;
};

#endif // LEVELMANAGER_HPP
