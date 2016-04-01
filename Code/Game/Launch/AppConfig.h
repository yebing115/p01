#pragma once

#include "Data/DataType.h"
#include "Data/String.h"

class GameApplication;
class AppConfig {
public:
  static String RESOURCES_FOLDER;
  static String WINDOW_TITLE;
  static String ICON_FILENAME;
  static bool USE_SYSTEM_CURSOR;
  static int MAX_FPS;
  static float2 RESOLUTION;
  static bool USE_FULL_SCREEN;
  static bool USE_VSYNC;
  //static GraphicsAPI GRAPHICS_API;
  static int MSAA_SAMPLES;
  static String LOG_FILE;

  static String START_SCRIPT;
  static bool START_HIDDEN;
  static bool USE_SRGB;
  static int AO_QUALITY;

  static void LoadConfig();
  static GameApplication* CreateApplication();
};
