#include "C3PCH.h"
#include "GameApplication.h"
#include "IGame.h"

GameApplication* g_app = nullptr;

GameApplication::GameApplication(IGame* game): _game(game), _paused(false) {
  g_app = this;
}

GameApplication::~GameApplication() {
  g_app = nullptr;
}

void GameApplication::Update(float dt) {
  InputManager::Instance()->Update(dt, _paused);
  if (_game) _game->OnUpdate(dt, _paused);
}

void GameApplication::Render(float dt) {
  if (_game) _game->OnRender(dt, _paused);
}

void GameApplication::Pause() {
  _paused = true;
}

bool GameApplication::IsPaused() const {
  return _paused;
}
