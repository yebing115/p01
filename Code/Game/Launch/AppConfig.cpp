#include "AppConfig.h"
#include "Game.h"
#include "Game/GameApplication.h"
#include "Debug/C3Debug.h"

String AppConfig::RESOURCES_FOLDER = "Assets";
String AppConfig::WINDOW_TITLE = "Untitled";
String AppConfig::ICON_FILENAME{"icon.ico"};
bool AppConfig::USE_SYSTEM_CURSOR = true;
int AppConfig::MAX_FPS = 12000;
float2 AppConfig::RESOLUTION{1600, 900};
bool AppConfig::USE_FULL_SCREEN = false;
bool AppConfig::USE_VSYNC = false;
int AppConfig::MSAA_SAMPLES = 1;
//GraphicsAPI AppConfig::GRAPHICS_API = OPENGL_WIN_API;
String AppConfig::LOG_FILE = "debug.log";

String AppConfig::START_SCRIPT = "game.lua";
bool AppConfig::START_HIDDEN = false;
bool AppConfig::USE_SRGB = false;
int AppConfig::AO_QUALITY = 1;

void AppConfig::LoadConfig() {
  LogManager::CreateInstance();
  auto LI = LogManager::Instance();
  LI->AddLogger(new DebugLogger);
  LI->AddLogger(new FileLogger(LOG_FILE));
  c3_log("[C3] Logger created.\n");
}

GameApplication* AppConfig::CreateApplication() {
  return new GameApplication(new Game);
}
