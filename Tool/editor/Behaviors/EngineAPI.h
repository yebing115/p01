#pragma once
#include "Pattern/Singleton.h"
#include "Platform/Windows/WindowsHeader.h"
#include <sciter-x.h>

class EngineAPI : public sciter::host<EngineAPI>
                , public sciter::event_handler
                , public sciter::debug_output {
public:
  EngineAPI(HWND hwnd);
  ~EngineAPI();

  // sciter::host traits
  HWINDOW get_hwnd() const { return _hwnd; }
  HINSTANCE get_resource_instance() const { return GetModuleHandle(NULL); }

  sciter::value api_test(unsigned argc, const sciter::value* arg);

  BEGIN_FUNCTION_MAP
    FUNCTION_V("api_test", api_test);
  END_FUNCTION_MAP
private:
  HWND _hwnd;
  SUPPORT_SINGLETON_WITH_ONE_ARG_CREATOR(EngineAPI, HWND);
};