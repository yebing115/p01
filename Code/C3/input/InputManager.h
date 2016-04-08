#pragma once

#include "Pattern/Singleton.h"
#include "KeyCodes.h"
#include "MouseCodes.h"

class InputManager {
public:
  InputManager();
  ~InputManager();

  bool IsKeyDown(int key_index) const;
  bool IsKeyPressed(int key_index, bool repeat = true) const;
  bool IsKeyReleased(int key_index) const;
  bool IsMouseDown(MouseButton button) const;
  bool IsMouseClicked(MouseButton button, bool repeat = false) const;
  bool IsMouseDoubleClicked(MouseButton button) const;
  bool IsMouseReleased(MouseButton button) const;
  bool IsMouseDragging(MouseButton button = LEFT_BUTTON, float lock_threshold = -1.0f) const;
  float2 GetMousePos() const;
  float2 GetMouseDragDelta(MouseButton button = LEFT_BUTTON, float lock_threshold = -1.0f) const;
  void ResetMouseDragDelta(MouseButton button = LEFT_BUTTON);

  void Update(float dt, bool paused);

  void _SetMousePos(float x, float y);
  void _SetMouseDown(MouseButton button);
  void _SetMouseUp(MouseButton button);
  void _SetMouseWheel(float wheel);
  void _SetKeyDown(int vkey);
  void _SetKeyUp(int vkey);
  void _SetKeyCtrl(bool down);
  void _SetKeyShift(bool down);
  void _SetKeyAlt(bool down);
  void _AddInputCharacter(wchar_t ch);

private:
  float2 _mouse_pos;
  bool _mouse_down[NUM_MOUSE_BUTTONS];
  float _mouse_wheel;
  bool _key_ctrl;
  bool _key_shift;
  bool _key_alt;
  bool _keys_down[512];
  wchar_t _input_characters[16 + 1];

  float _dt;
  double _time;
  float2 _mouse_pos_prev;
  float2 _mouse_delta;
  bool _mouse_clicked[NUM_MOUSE_BUTTONS];
  float2 _mouse_clicked_pos[NUM_MOUSE_BUTTONS];
  float _mouse_clicked_time[NUM_MOUSE_BUTTONS];
  bool _mouse_double_clicked[NUM_MOUSE_BUTTONS];
  bool _mouse_released[NUM_MOUSE_BUTTONS];
  float _mouse_down_duration[NUM_MOUSE_BUTTONS];
  float _mouse_down_duration_prev[NUM_MOUSE_BUTTONS];
  float _mouse_drag_max_distance_sqr[NUM_MOUSE_BUTTONS];
  float _keys_down_duration[512];
  float _keys_down_duration_prev[512];

  SUPPORT_SINGLETON(InputManager);
};
