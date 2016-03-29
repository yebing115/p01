#include "Debug/C3Debug.h"
#include "Platform/Windows/WindowsHeader.h"
#include "Platform/PlatformData.h"
#include "Text/EncodingUtil.h"
#include "AppConfig.h"
#include "Reflection/Reflector.h"
#include "Reflection/Object.h"
#include "Reflection/ReflectMethod.h"
#include "Game/GameApplication.h"
#include "Graphics/GraphicsRenderer.h"
#include "Graphics/imgui_helpers.h"
#include <shellapi.h>
#include <thread>

#pragma execution_character_set("utf-8")

static LPCTSTR WINDOW_CLASS_NAME = __TEXT("Main");
static const float FIRST_FRAME_MIN_TIME = 0.01f;
extern HDC g_dc = nullptr;
static HWND s_hwnd = nullptr;
static float s_min_frame_time;
static float2 s_window_size(0, 0);
static bool s_active = false;
static float2 s_mouse_position(-1, -1);
static bool s_mouse_pressed[3];
static float s_mouse_wheel;
static String s_text;
static GameApplication* s_app;

typedef LRESULT (CALLBACK *WndProc)(HWND, UINT, WPARAM, LPARAM);

void SetResourcesFolder();
ATOM RegisterWindowClass(HINSTANCE hInstance, LPCTSTR class_name, LPCTSTR icon_filename, WndProc wnd_proc);
LRESULT CALLBACK WndProcMain(HWND, UINT, WPARAM, LPARAM);
bool InitWindow(HINSTANCE hInstance, int nCmdShow);

void ParseCommandLine(LPTSTR /*lpCmdLine*/) {
  int argc = 0;
  auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  for (int i = 0; i < argc; ++i) g_platform_data.arguments.emplace_back((wchar_t*)argv[i]);
}

class Q: public Object {
public:
  float K(int i) { 
    return float(i);
  }
  int q;
};
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE /*hPrevInstance*/,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int      nCmdShow) {

  Reflector r;
  //r.Reflect("k", Q::K);
  r.Reflect("k", &Q::q);

  SetThreadIdealProcessor(GetCurrentThread(), 0);

  ParseCommandLine(lpCmdLine);
  SetResourcesFolder();
  AppConfig::LoadConfig();

  mem_init();

  GraphicsRenderer::CreateInstance();
  if (!InitWindow(hInstance, nCmdShow) ||
      !GraphicsRenderer::Instance()->Init(D3D11_WIN_API)) {
    return false;
  }

  s_app = AppConfig::CreateApplication();
  imgui_init(s_hwnd);

  tick_t this_frame_start_time = Clock::Tick();
  tick_t last_frame_start_time;
  s_min_frame_time = 1.0f / AppConfig::MAX_FPS;
  bool first_frame = true;

  auto& IO = ImGui::GetIO();
  MSG msg;
  while (1) {
    last_frame_start_time = this_frame_start_time;
    this_frame_start_time = Clock::Tick();
    float dt = float(this_frame_start_time - last_frame_start_time) / Clock::TicksPerSec();
    float min_time = first_frame ? FIRST_FRAME_MIN_TIME : s_min_frame_time;
    if (first_frame) first_frame = false;
    while (dt < min_time) {
      std::this_thread::yield();
      this_frame_start_time = Clock::Tick();
      dt = float(this_frame_start_time - last_frame_start_time) / Clock::TicksPerSec();
    }

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        exit(0);
        break;
      } else {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
    //ProcessXInput();

    GraphicsRenderer::Instance()->Reset((u16)s_window_size.x, (u16)s_window_size.y,
                                        C3_RESET_FULLSCREEN);

    IO.KeyCtrl = (GetKeyState(VK_LCONTROL) & 0x8000) || (GetKeyState(VK_RCONTROL) & 0x8000);
    IO.KeyShift = (GetKeyState(VK_LSHIFT) & 0x8000) || (GetKeyState(VK_RSHIFT) & 0x8000);
    IO.KeyAlt = (GetKeyState(VK_LMENU) & 0x8000) || (GetKeyState(VK_RMENU) & 0x8000);
    imgui_new_frame(s_window_size, s_mouse_position, s_mouse_pressed, s_mouse_wheel, dt);
    s_mouse_wheel = 0.f;

    s_app->Update(dt);
    s_app->Render(dt);

    ImGui::Text("Hello, world!");
    ImGui::ShowTestWindow();
    ImGui::Render();

    GraphicsRenderer::Instance()->Frame();
  }

  return (int) msg.wParam;
}

void SetResourcesFolder() {
  auto folder_system = EncodingUtil::UTF8ToSystem(AppConfig::RESOURCES_FOLDER);
  SetCurrentDirectory((LPCTSTR)folder_system.GetCString());
}

ATOM RegisterWindowClass(HINSTANCE hInstance, LPCTSTR class_name, LPCTSTR icon_filename, WndProc wnd_proc) {
  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
  wcex.lpfnWndProc = wnd_proc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = icon_filename ? (HICON) LoadImage(nullptr, icon_filename, IMAGE_ICON, 64, 64, LR_LOADFROMFILE) : nullptr;
  wcex.hIconSm = icon_filename ? (HICON) LoadImage(nullptr, icon_filename, IMAGE_ICON, 16, 16, LR_LOADFROMFILE) : nullptr;
  wcex.hCursor = AppConfig::USE_SYSTEM_CURSOR ? LoadCursor(nullptr, IDC_ARROW) : nullptr;
  wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
  wcex.lpszMenuName = nullptr; // MAKEINTRESOURCE(IDC_TRYWIN32);
  wcex.lpszClassName = class_name;

  return RegisterClassEx(&wcex);
}

static void RegisterSystemHotKeys(HWND hWnd) {
  RegisterHotKey(hWnd, 0, MOD_ALT, VK_F4);
}

/*
static void UnregisterSystemHotKeys(HWND hWnd) {
  UnregisterHotKey(hWnd, 0);
}
*/

static bool ComputeWindowRect(RECT& out_rect) {
  int monitor_width, monitor_height;
  int width, height;
  if (AppConfig::USE_FULL_SCREEN) {
    monitor_width = GetSystemMetrics(SM_CXSCREEN);
    monitor_height = GetSystemMetrics(SM_CYSCREEN);
    width = (AppConfig::RESOLUTION[0] <= 1) ? (int)(AppConfig::RESOLUTION[0] * monitor_width) : (int)AppConfig::RESOLUTION[0];
    height = (AppConfig::RESOLUTION[1] <= 1) ? (int)(AppConfig::RESOLUTION[1] * monitor_height) : (int)AppConfig::RESOLUTION[1];
    // choose best display mode
    vector<DEVMODE> avail_modes;
    DEVMODE mode;
    mode.dmSize = sizeof(DEVMODE);
    mode.dmDriverExtra = 0;
    mode.dmBitsPerPel = 0;
    DWORD i = 0;
    while (EnumDisplaySettings(NULL, i++, &mode)) {
      avail_modes.push_back(mode);
    }
    if (avail_modes.empty()) return false;
    int diff_w = 9999, diff_h = 9999;
    for (auto& m : avail_modes) {
      int dw = std::abs((int)m.dmPelsWidth - width);
      int dh = std::abs((int)m.dmPelsHeight - height);
      if (dw < diff_w || dw == diff_w && dh < diff_h) {
        diff_w = dw;
        diff_h = dh;
        mode = m;
      }
    }
    mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    mode.dmBitsPerPel = 32;
    auto ret = ChangeDisplaySettings(&mode, CDS_FULLSCREEN);
    if (ret != DISP_CHANGE_SUCCESSFUL) return false;
    monitor_width = width = mode.dmPelsWidth;
    monitor_height = height = mode.dmPelsHeight;
  } else {
    RECT work_area;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
    monitor_width = work_area.right;
    monitor_height = work_area.bottom;
    width = (AppConfig::RESOLUTION[0] <= 1) ? (int)(AppConfig::RESOLUTION[0] * monitor_width) : (int)AppConfig::RESOLUTION[0];
    height = (AppConfig::RESOLUTION[1] <= 1) ? (int)(AppConfig::RESOLUTION[1] * monitor_height) : (int)AppConfig::RESOLUTION[1];
  }
  out_rect.left = (monitor_width - width) / 2;
  out_rect.right = out_rect.left + width;
  out_rect.top = (monitor_height - height) / 2;
  out_rect.bottom = out_rect.top + height;
  return true;
}

bool InitWindow(HINSTANCE hInstance, int nCmdShow) {
  //if (!InitGLEW(hInstance)) return false;

  String title = EncodingUtil::UTF8ToSystem(AppConfig::WINDOW_TITLE);
  String icon_filename = EncodingUtil::UTF8ToSystem(AppConfig::ICON_FILENAME);
  RegisterWindowClass(hInstance, WINDOW_CLASS_NAME, (LPCTSTR)icon_filename.GetCString(), WndProcMain);
  DWORD style = AppConfig::USE_FULL_SCREEN ? (WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP) : WS_OVERLAPPEDWINDOW;
  if (AppConfig::START_HIDDEN) style = WS_POPUP;
  RECT rect;
  if (!ComputeWindowRect(rect)) {
    c3_log("不支持的分辨率设置。\n");
    SetRect(&rect, 0, 0, 800, 600);
  }
  s_window_size.Set(float(rect.right - rect.left), float(rect.bottom - rect.top));
  AdjustWindowRect(&rect, style, FALSE);
  s_hwnd = CreateWindow(WINDOW_CLASS_NAME, (LPCTSTR)title.GetCString(), style,
    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
    NULL, NULL, hInstance, NULL);
  if (!s_hwnd) return false;
  g_platform_data.nwh = (void*)s_hwnd;
  g_dc = GetDC(s_hwnd);
  RegisterSystemHotKeys(s_hwnd);
  if (!AppConfig::START_HIDDEN) ShowWindow(s_hwnd, nCmdShow);
  UpdateWindow(s_hwnd);

  return true;
}

/*
static void ProcessSystemHotKeys(HWND hWnd, WORD vkey, WORD mods) {
  if ((mods & MOD_ALT) && vkey == VK_F4) ::SendMessage(hWnd, WM_CLOSE, NULL, NULL);
}
*/

LRESULT CALLBACK WndProcMain(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  auto& IO = ImGui::GetIO();
  switch (message) {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_KEYDOWN:
      IO.KeysDown[wParam] = true;
      break;
    case WM_KEYUP:
      IO.KeysDown[wParam] = false;
      break;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
      s_mouse_pressed[0] = true;
      break;
    case WM_LBUTTONUP:
      s_mouse_pressed[0] = false;
      break;
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
      s_mouse_pressed[2] = true;
      break;
    case WM_MBUTTONUP:
      s_mouse_pressed[2] = false;
      break;
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
      s_mouse_pressed[1] = true;
      break;
    case WM_RBUTTONUP:
      s_mouse_pressed[1] = false;
      break;
    case WM_MOUSEMOVE:
      s_mouse_position.Set((float)GET_X_LPARAM(lParam), s_window_size.y - (float)GET_Y_LPARAM(lParam));
      break;
    case WM_MOUSEWHEEL:
      s_mouse_wheel = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
      break;
    case WM_ACTIVATE:
      if (wParam == WA_INACTIVE) {
      } else {
      }
      break;
    case WM_SIZE: {
      s_window_size.Set((float) LOWORD(lParam), (float) HIWORD(lParam));
      break;
    }
    case WM_CHAR:
      if (wParam > 0 && wParam < 0x10000) IO.AddInputCharacter((ImWchar)wParam);
      break;
    case WM_HOTKEY:
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}
