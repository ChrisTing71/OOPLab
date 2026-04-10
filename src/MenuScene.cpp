#include "MenuScene.hpp"

#include "LevelManager.hpp"
#include "Util/Input.hpp"
#include "config.hpp"

MenuScene::MenuScene(std::shared_ptr<LevelManager> levelManager)
    : m_LevelManager(levelManager), m_HighlightedIndex(0) {
  CreateLevelButtons();
}

void MenuScene::CreateLevelButtons() {
  m_LevelButtons.clear();

  // Create 10 level buttons in a 5x2 grid layout
  float startX = 200.0F;
  float startY = 150.0F;
  float buttonWidth = 150.0F;
  float buttonHeight = 100.0F;
  float spacingX = 200.0F;
  float spacingY = 150.0F;

  for (int i = 1; i <= 10; ++i) {
    LevelButton btn;
    btn.levelId = i;
    btn.displayName = "Level " + std::to_string(i);
    btn.unlocked = true; // For now, all levels are unlocked

    // Simple difficulty progression
    if (i <= 3) {
      btn.difficulty = "Easy";
    } else if (i <= 6) {
      btn.difficulty = "Medium";
    } else {
      btn.difficulty = "Hard";
    }

    // Calculate grid position (5 columns, 2 rows)
    int col = (i - 1) % 5;
    int row = (i - 1) / 5;

    btn.position = {startX + col * spacingX, startY + row * spacingY};
    btn.size = {buttonWidth, buttonHeight};

    m_LevelButtons.push_back(btn);
  }
}

int MenuScene::Update(float /**deltaTime*/) {
  UpdateInput();

  // Return selected level ID if one was picked
  if (m_SelectedLevelId > 0) {
    int result = m_SelectedLevelId;
    m_SelectedLevelId = 0; // Reset for next use
    return result;
  }

  return 0; // No selection
}

void MenuScene::UpdateInput() {
  // Keyboard navigation (arrow keys or WASD)
  if (Util::Input::IsKeyPressed(Util::Keycode::LEFT) ||
      Util::Input::IsKeyPressed(Util::Keycode::A)) {
    m_HighlightedIndex = (m_HighlightedIndex - 1 + m_LevelButtons.size()) %
                         m_LevelButtons.size();
  }
  if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT) ||
      Util::Input::IsKeyPressed(Util::Keycode::D)) {
    m_HighlightedIndex = (m_HighlightedIndex + 1) % m_LevelButtons.size();
  }
  if (Util::Input::IsKeyPressed(Util::Keycode::UP) ||
      Util::Input::IsKeyPressed(Util::Keycode::W)) {
    if (m_HighlightedIndex >= 5) {
      m_HighlightedIndex -= 5; // Move up one row
    }
  }
  if (Util::Input::IsKeyPressed(Util::Keycode::DOWN) ||
      Util::Input::IsKeyPressed(Util::Keycode::S)) {
    if (m_HighlightedIndex < 5) {
      m_HighlightedIndex += 5; // Move down one row
    }
  }

  // Select level with ENTER or SPACE
  if (Util::Input::IsKeyPressed(Util::Keycode::RETURN) ||
      Util::Input::IsKeyPressed(Util::Keycode::SPACE)) {
    m_SelectedLevelId = m_LevelButtons[m_HighlightedIndex].levelId;
  }

  // Mouse input
  if (Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB)) {
    for (size_t i = 0; i < m_LevelButtons.size(); ++i) {
      if (IsMouseOverButton(m_LevelButtons[i])) {
        m_SelectedLevelId = m_LevelButtons[i].levelId;
        m_HighlightedIndex = i;
        break;
      }
    }
  }

  // Mouse hover highlight
  for (size_t i = 0; i < m_LevelButtons.size(); ++i) {
    if (IsMouseOverButton(m_LevelButtons[i])) {
      m_HighlightedIndex = i;
      break;
    }
  }
}

void MenuScene::Render(float /**deltaTime*/) { RenderLevelSelect(); }

void MenuScene::RenderLevelSelect() {
  // Render title using ImGui
  ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH * 0.1F, 20.0F), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH * 0.8F, WINDOW_HEIGHT - 40.0F),
                           ImGuiCond_Always);
  ImGui::Begin("Select a Level", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

  ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 400);
  ImGui::TextUnformatted("Choose a level to play:");
  ImGui::PopTextWrapPos();
  ImGui::Spacing();
  ImGui::Spacing();

  // Draw level buttons as a grid
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 5; ++col) {
      int idx = row * 5 + col;
      if (idx >= static_cast<int>(m_LevelButtons.size()))
        break;

      const auto &btn = m_LevelButtons[idx];
      std::string label =
          "Level " + std::to_string(btn.levelId) + "\n[" + btn.difficulty + "]";

      bool highlighted = (idx == static_cast<int>(m_HighlightedIndex));
      if (highlighted) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4F, 0.8F, 0.4F, 1.0F));
      }

      if (ImGui::Button(label.c_str(), ImVec2(120.0F, 80.0F))) {
        m_SelectedLevelId = btn.levelId;
      }

      if (highlighted) {
        ImGui::PopStyleColor();
      }

      if (col < 4) {
        ImGui::SameLine();
      }
    }
    ImGui::Spacing();
  }

  ImGui::Spacing();
  ImGui::TextDisabled("Use ARROW KEYS or click to select, ENTER to start");

  ImGui::End();
}

bool MenuScene::IsMouseOverButton(const LevelButton &button) const {
  glm::vec2 mousePos = Util::Input::GetCursorPosition();
  // Convert cursor coordinates (window-centered) to screen pixel coordinates
  float mousePixelX = mousePos.x + static_cast<float>(WINDOW_WIDTH) * 0.5F;
  float mousePixelY = static_cast<float>(WINDOW_HEIGHT) * 0.5F - mousePos.y;

  return mousePixelX >= button.position.x &&
         mousePixelX <= button.position.x + button.size.x &&
         mousePixelY >= button.position.y &&
         mousePixelY <= button.position.y + button.size.y;
}

void MenuScene::Reset() {
  m_SelectedLevelId = 0;
  m_PreviewingLevelId = 0;
  m_CurrentState = MenuState::LEVEL_SELECT;
  m_TransitionProgress = 0.0F;
  m_HighlightedIndex = 0;
}
