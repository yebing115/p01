#include "Game.h"
#include "AppConfig.h"
#include "C3PCH.h"
#include "Graphics/RenderSystem.h"

Game::Game() {
  auto world = GameWorld::CreateInstance();
  auto e = world->CreateEntity();
  world->CreateCamera(e);
  world->CreateTransform(e);
  auto renderer = new RenderSystem;
  world->AddSystem(renderer);
  world->CreateComponent(e, MODEL_RENDERER_HANDLE);
  renderer->SetModelFilename(e, "Models/sponza.mex");
}

void Game::OnUpdate(float dt, bool paused) {
  (void)dt;
  (void)paused;
  GameWorld::Instance()->Update(dt, paused);
}

void Game::OnRender(float dt, bool paused) {
  (void)dt;
  (void)paused;

  GameWorld::Instance()->Render(dt, paused);
  return;

  auto GR = GraphicsRenderer::Instance();
  auto view = GR->PushView();
  auto win_size = GR->GetWindowSize();
  GR->SetViewRect(view, 0, 0, win_size.x, win_size.y);
  GR->SetViewClear(view, C3_CLEAR_COLOR | C3_CLEAR_DEPTH, 0x808080ff, 1.f);
  GR->Touch(view);
  GR->PopView();
}
