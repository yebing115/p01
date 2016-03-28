#pragma once

class IGame;
class GameApplication {
public:
  GameApplication(IGame* game);
  virtual ~GameApplication();
  void Update(float dt);
  void Render(float dt);
  void Pause();
  bool IsPaused() const;
private:
  IGame* _game;
  bool _paused;
};

extern GameApplication* g_app;
