#pragma once
#include "Game/IGame.h"
#include "Asset/AssetManager.h"

class Game : public IGame {
public:
  Game();
  void OnUpdate(float dt, bool paused) override;

  void OnRender(float dt, bool paused) override;

private:
  Asset* _model;
  Asset* _texture;
};
