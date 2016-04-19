#include "C3PCH.h"
#include "GameWorldEventHandler.h"
using namespace sciter;

extern HWND g_game_world_hwnd;

struct GameWorldFactory : public behavior_factory {

  GameWorldFactory() : behavior_factory("game_world") {}

  // the only behavior_factory method:
  event_handler* create(HELEMENT he) override { return new GameWorldEventHandler(); }

};

// instantiating and attaching it to the global list of supported behaviors
static GameWorldFactory game_world_factor_instance;

void GameWorldEventHandler::attached(HELEMENT he) {
  dom::element el = he;
  el.attach_hwnd(g_game_world_hwnd);
}

void GameWorldEventHandler::detached(HELEMENT he) {
  dom::element el = he;
  el.attach_hwnd(NULL);
  delete this;
}
