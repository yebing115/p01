#pragma once

class IGame {
public:
  virtual void OnUpdate(float dt, bool paused) = 0;
  virtual void OnRender(float dt, bool paused) = 0;
};
