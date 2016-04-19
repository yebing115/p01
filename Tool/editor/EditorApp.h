#pragma once
#include "Game/IGame.h"
#include "Asset/AssetManager.h"

class EditorApp : public IGame {
public:
  EditorApp();
  void OnUpdate(float dt, bool paused) override;
  void OnRender(float dt, bool paused) override;

private:
  void UpdateDebugCamera(float dt);

  EntityHandle _debug_camera;
  EntityHandle _model;
};
