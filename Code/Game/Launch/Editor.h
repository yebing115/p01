#pragma once
#include "Game/IGame.h"
#include "Asset/AssetManager.h"

class Editor : public IGame {
public:
  Editor();
  void OnUpdate(float dt, bool paused) override;
  void OnRender(float dt, bool paused) override;

private:
  void ShowMenuFile();
  void ShowMenuEdit();
  void UpdateDebugCamera(float dt);

  EntityHandle _debug_camera;
  EntityHandle _model;
};
