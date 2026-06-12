#include "App.hpp"

#include "Core/Context.hpp"

int main(int, char **) {
  auto context = Core::Context::GetInstance();
  App app;

  while (!context->GetExit()) {
    context->Setup();

    switch (app.GetCurrentState()) {
    case App::State::START:
      app.Start();
      break;

    case App::State::MENU:
    case App::State::GAME_LOADING:
    case App::State::PLAYING:
    case App::State::PAUSED:
    case App::State::LEVEL_COMPLETE:
    case App::State::LEVEL_FAILED:
    case App::State::GAME_OVER:
      app.Update();
      break;

    case App::State::UPDATE:
      // Legacy state - should not be used with new system
      app.Update();
      break;

    case App::State::END:
      app.End();
      context->SetExit(true);
      break;
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    context->Update();
  }
  return 0;
}
