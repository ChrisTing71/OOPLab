#ifndef MENUSCENE_HPP
#define MENUSCENE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Util/Renderer.hpp"
#include "pch.hpp"

class LevelManager;

/**
 * Menu scene for level selection
 */
class MenuScene {
public:
  explicit MenuScene(std::shared_ptr<LevelManager> levelManager);
  ~MenuScene() = default;

  enum class MenuState {
    LEVEL_SELECT,  // Showing level selection grid
    LEVEL_PREVIEW, // Showing level preview with details
    CONFIRM_START, // Confirmation before starting level
    EXITING,       // Transitioning away from menu
  };

  /**
   * Handle input and update menu state
   * @return The selected level ID if a level was selected (1-10), or 0 if no
   * selection
   */
  int Update(float deltaTime);

  /**
   * Render menu UI
   */
  void Render(float deltaTime);

  /**
   * Check if user selected a level
   */
  bool IsLevelSelected() const { return m_SelectedLevelId > 0; }

  /**
   * Get the selected level ID
   */
  int GetSelectedLevelId() const { return m_SelectedLevelId; }

  /**
   * Reset menu state
   */
  void Reset();

private:
  struct LevelButton {
    int levelId = 0;
    std::string displayName;
    std::string difficulty;
    bool unlocked = false;
    bool completed = false;
    glm::vec2 position = {0.0F, 0.0F};
    glm::vec2 size = {0.0F, 0.0F};
  };

  std::shared_ptr<LevelManager> m_LevelManager;
  std::vector<LevelButton> m_LevelButtons;
  int m_SelectedLevelId = 0;
  int m_PreviewingLevelId = 0;
  MenuState m_CurrentState = MenuState::LEVEL_SELECT;
  float m_TransitionProgress = 0.0F;
  int m_HighlightedIndex = 0;

  void CreateLevelButtons();
  void UpdateInput();
  void RenderLevelSelect();
  void RenderLevelPreview(int levelId);
  void RenderConfirmation();

  bool IsMouseOverButton(const LevelButton &button) const;
};

#endif // MENUSCENE_HPP
