#pragma once
#include "Game/IGame.h"

class Game : public IGame {
public:
  void OnUpdate(float dt, bool paused) override;

  void OnRender(float dt, bool paused) override;

};
