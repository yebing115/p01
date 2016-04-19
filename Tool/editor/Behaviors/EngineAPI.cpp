#include "EngineAPI.h"
using namespace sciter;

DEFINE_SINGLETON_INSTANCE(EngineAPI);

EngineAPI::EngineAPI(HWND hwnd): debug_output(hwnd), _hwnd(hwnd) {
  setup_callback(hwnd);
  attach_dom_event_handler(hwnd, this);
  SciterSetHomeURL(hwnd, L"file://../Tool/editor/res");
}

EngineAPI::~EngineAPI() {

}

value EngineAPI::api_test(unsigned argc, const value* arg) {
  return value::from_string(string(L"{\"d\":\"qqq\"}"), CVT_JSON_LITERAL);
}
