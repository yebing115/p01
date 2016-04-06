#include "Game.h"
#include "AppConfig.h"
#include "C3PCH.h"

Game::Game() {
  _model = AssetManager::Instance()->Load(ASSET_TYPE_MODEL, "Models/sponza.mex");
  _texture = AssetManager::Instance()->Load(ASSET_TYPE_TEXTURE, "Models/textures/background.dds");
  GameWorld::CreateInstance();
}

void Game::OnUpdate(float dt, bool paused) {
  (void)dt;
  (void)paused;
}

void Game::OnRender(float dt, bool paused) {
  (void)dt;
  (void)paused;
  auto GR = GraphicsRenderer::Instance();
  auto view = GR->PushView();
  auto win_size = GR->GetWindowSize();
  GR->SetViewRect(view, 0, 0, win_size.x, win_size.y);
  GR->SetViewClear(view, C3_CLEAR_COLOR | C3_CLEAR_DEPTH, 0x808080ff, 1.f);
  GR->Touch(view);
  GR->PopView();
}
