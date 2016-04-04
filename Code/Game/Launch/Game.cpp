#include "Game.h"
#include "AppConfig.h"
#include "C3PCH.h"

Game::Game() {
  _model = AssetManager::Instance()->Load(ASSET_TYPE_MODEL, "Models/sponza.mex");
  _texture = AssetManager::Instance()->Load(ASSET_TYPE_TEXTURE, "Models/textures/background.dds");
}

void Game::OnUpdate(float dt, bool paused) {
  (void)dt;
  (void)paused;
}

void Game::OnRender(float dt, bool paused) {
  (void)dt;
  (void)paused;
}
