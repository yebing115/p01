#include "C3PCH.h"
#include "InputManager.h"

DEFINE_SINGLETON_INSTANCE(InputManager);

static const float MOUSE_DOUBLE_CLICK_MAX_DIST = 6.f;
static const float MOUSE_DOUBLE_CLICK_TIME = .3f;
static const float KEY_REPEAT_DELAY = 0.25f;
static const float KEY_REPEAT_RATE = 0.02f;
static const float MOUSE_DRAG_THREHOLD = 6.f;

InputManager::InputManager() {
  _mouse_pos.Set(-1.f, -1.f);
  memset(_mouse_down, 0, sizeof(_mouse_down));
  _mouse_wheel = 0.f;
  _key_ctrl = false;
  _key_shift = false;
  _key_alt = false;
  memset(_keys_down, 0, sizeof(_keys_down));
  memset(_input_characters, 0, sizeof(_input_characters));

  _dt = 0.f;
  _time = 0;
  _mouse_pos_prev.Set(-1.f, -1.f);
  _mouse_delta.Set(0.f, 0.f);
  memset(_mouse_clicked, 0, sizeof(_mouse_clicked));
  memset(_mouse_clicked_time, 0, sizeof(_mouse_clicked_time));
  memset(_mouse_double_clicked, 0, sizeof(_mouse_double_clicked));
  memset(_mouse_released, 0, sizeof(_mouse_released));
  memset(_mouse_down_duration, 0, sizeof(_mouse_down_duration));
  memset(_mouse_down_duration_prev, 0, sizeof(_mouse_down_duration_prev));
  memset(_mouse_drag_max_distance_sqr, 0, sizeof(_mouse_drag_max_distance_sqr));
  memset(_keys_down_duration, 0, sizeof(_keys_down_duration));
  memset(_keys_down_duration_prev, 0, sizeof(_keys_down_duration_prev));
}

InputManager::~InputManager() {

}

bool InputManager::IsKeyDown(int key_index) const {
  c3_assert_return_x(key_index >= 0 && key_index < ARRAY_SIZE(_keys_down), false);
  return _keys_down[key_index];
}

bool InputManager::IsKeyPressed(int key_index, bool repeat) const {
  c3_assert_return_x(key_index >= 0 && key_index < ARRAY_SIZE(_keys_down), false);
  const float t = _keys_down_duration[key_index];
  if (t == 0.0f) return true;

  if (repeat && t > KEY_REPEAT_DELAY) {
    float delay = KEY_REPEAT_DELAY, rate = KEY_REPEAT_RATE;
    if ((fmodf(t - delay, rate) > rate * 0.5f) != (fmodf(t - delay - _dt, rate) > rate * 0.5f)) {
      return true;
    }
  }
  return false;
}

bool InputManager::IsKeyReleased(int key_index) const {
  c3_assert_return_x(key_index >= 0 && key_index < ARRAY_SIZE(_keys_down), false);
  if (_keys_down_duration_prev[key_index] >= 0.0f && !_keys_down[key_index]) return true;
  return false;
}

bool InputManager::IsMouseDown(MouseButton button) const {
  c3_assert_return_x((int)button >= 0 && (int)button < NUM_MOUSE_BUTTONS, false);
  return _mouse_down[button];
}

bool InputManager::IsMouseClicked(MouseButton button, bool repeat) const {
  c3_assert_return_x((int)button >= 0 && (int)button < NUM_MOUSE_BUTTONS, false);
  const float t = _mouse_down_duration[button];
  if (t == 0.0f) return true;

  if (repeat && t > KEY_REPEAT_DELAY) {
    float delay = KEY_REPEAT_DELAY, rate = KEY_REPEAT_RATE;
    if ((fmodf(t - delay, rate) > rate * 0.5f) != (fmodf(t - delay - _dt, rate) > rate * 0.5f)) {
      return true;
    }
  }

  return false;
}

bool InputManager::IsMouseDoubleClicked(MouseButton button) const {
  c3_assert_return_x((int)button >= 0 && (int)button < NUM_MOUSE_BUTTONS, false);
  return _mouse_double_clicked[button];
}

bool InputManager::IsMouseReleased(MouseButton button) const {
  c3_assert_return_x((int)button >= 0 && (int)button < NUM_MOUSE_BUTTONS, false);
  return _mouse_released[button];
}

bool InputManager::IsMouseDragging(MouseButton button, float lock_threshold) const {
  c3_assert_return_x((int)button >= 0 && (int)button < NUM_MOUSE_BUTTONS, false);
  if (!_mouse_down[button]) return false;
  if (lock_threshold < 0.0f) lock_threshold = MOUSE_DRAG_THREHOLD;
  return _mouse_drag_max_distance_sqr[button] >= lock_threshold * lock_threshold;
}

float2 InputManager::GetMousePos() const {
  return _mouse_pos;
}

float2 InputManager::GetMouseDragDelta(MouseButton button, float lock_threshold) const {
  c3_assert_return_x((int)button >= 0 && (int)button < NUM_MOUSE_BUTTONS, float2::zero);
  if (lock_threshold < 0.0f) lock_threshold = MOUSE_DRAG_THREHOLD;
  if (_mouse_down[button]) {
    if (_mouse_drag_max_distance_sqr[button] >= lock_threshold * lock_threshold) {
      // Assume we can only get active with left-mouse button (at the moment).
      return _mouse_pos - _mouse_clicked_pos[button];
    }
  }
  return float2::zero;
}

void InputManager::ResetMouseDragDelta(MouseButton button) {
  c3_assert_return((int)button >= 0 && (int)button < NUM_MOUSE_BUTTONS);
  _mouse_clicked_pos[button] = _mouse_pos;
}

void InputManager::Update(float dt, bool paused) {
  _dt = dt;
  _time += dt;
  // Update inputs state
  if (_mouse_pos.x < 0 && _mouse_pos.y < 0)
    _mouse_pos.Set(-9999.0f, -9999.0f);
  if ((_mouse_pos.x < 0 && _mouse_pos.y < 0) || (_mouse_pos_prev.x < 0 && _mouse_pos_prev.y < 0))   // if mouse just appeared or disappeared (negative coordinate) we cancel out movement in MouseDelta
    _mouse_delta.Set(0.0f, 0.0f);
  else
    _mouse_delta = _mouse_pos - _mouse_pos_prev;
  _mouse_pos_prev = _mouse_pos;
  for (int i = 0; i < ARRAY_SIZE(_mouse_down); i++) {
    _mouse_clicked[i] = _mouse_down[i] && _mouse_down_duration[i] < 0.0f;
    _mouse_released[i] = !_mouse_down[i] && _mouse_down_duration[i] >= 0.0f;
    _mouse_down_duration_prev[i] = _mouse_down_duration[i];
    _mouse_down_duration[i] = _mouse_down[i] ? (_mouse_down_duration[i] < 0.0f ? 0.0f : _mouse_down_duration[i] + dt) : -1.0f;
    _mouse_double_clicked[i] = false;
    if (_mouse_clicked[i]) {
      if (_time - _mouse_clicked_time[i] < MOUSE_DOUBLE_CLICK_TIME) {
        if ((_mouse_pos - _mouse_clicked_pos[i]).LengthSq() < MOUSE_DOUBLE_CLICK_MAX_DIST * MOUSE_DOUBLE_CLICK_MAX_DIST)
          _mouse_double_clicked[i] = true;
        _mouse_clicked_time[i] = -FLT_MAX;    // so the third click isn't turned into a double-click
      } else {
        _mouse_clicked_time[i] = _time;
      }
      _mouse_clicked_pos[i] = _mouse_pos;
      _mouse_drag_max_distance_sqr[i] = 0.0f;
    } else if (_mouse_down[i]) {
      _mouse_drag_max_distance_sqr[i] = max(_mouse_drag_max_distance_sqr[i], (_mouse_pos - _mouse_clicked_pos[i]).LengthSq());
    }
  }
  memcpy(_keys_down_duration_prev, _keys_down_duration, sizeof(_keys_down_duration));
  for (int i = 0; i < ARRAY_SIZE(_keys_down); i++) {
    _keys_down_duration[i] = _keys_down[i] ? (_keys_down_duration[i] < 0.0f ? 0.0f : _keys_down_duration[i] + dt) : -1.0f;
  }
}

void InputManager::_SetMousePos(float x, float y) {
  _mouse_pos.Set(x, y);
}

void InputManager::_SetMouseDown(MouseButton button) {
  _mouse_down[button] = true;
}

void InputManager::_SetMouseUp(MouseButton button) {
  _mouse_down[button] = false;
}

void InputManager::_SetMouseWheel(float wheel) {
  _mouse_wheel = wheel;
}

void InputManager::_SetKeyDown(int vkey) {
  if (vkey >= ARRAY_SIZE(_keys_down)) return;
  _keys_down[vkey] = true;
}

void InputManager::_SetKeyUp(int vkey) {
  if (vkey >= ARRAY_SIZE(_keys_down)) return;
  _keys_down[vkey] = false;
}

void InputManager::_SetKeyCtrl(bool down) {
  _key_ctrl = down;
}

void InputManager::_SetKeyShift(bool down) {
  _key_shift = down;
}

void InputManager::_SetKeyAlt(bool down) {
  _key_alt = down;
}

void InputManager::_AddInputCharacter(wchar_t ch) {
  auto n = wcslen(_input_characters);
  if (n >= ARRAY_SIZE(_input_characters) - 1) return;
  _input_characters[n] = ch;
  _input_characters[n + 1] = 0;
}
