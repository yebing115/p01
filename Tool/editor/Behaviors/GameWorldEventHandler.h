#pragma once

#include <sciter-x.h>

class GameWorldEventHandler : public sciter::event_handler {
public:
  void attached(HELEMENT he) override;
  void detached(HELEMENT he) override;
};

