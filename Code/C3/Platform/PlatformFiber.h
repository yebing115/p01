#pragma once
#include "Data/C3Data.h"
#include "Platform/PlatformConfig.h"
#if ON_WINDOWS
#include "Platform/Windows/WindowsHeader.h"
#endif

enum FiberState {
  FIBER_STATE_INITIALIZED,
  FIBER_STATE_RUNNING,
  FIBER_STATE_SUSPENDED,
  FIBER_STATE_FINISHED,
};

class Fiber {
public:
  Fiber();
  typedef i32(*FiberFn)(void* user_data);
  // INITIALIZED -> SUSPENDED
  void Prepare(FiberFn fn, void* data);
  // RUNNING -> SUSPENDED, run thread major fiber
  void Suspend();
  // RUNNING -> FINISHED, run thread major fiber
  void Finish();
  // SUSPENDED -> RUNNING, start self fiber code.
  void Resume();
  void* GetData() const { return _user_data; }
  FiberState GetState() const { return _state; }
  void SetState(FiberState state) { _state = state; }

  static Fiber* ConvertFromThread(void* user_data);
  static Fiber* GetScheduleFiber();
  static void SetScheduleFiber(Fiber* fiber);
  static Fiber* GetCurrentFiber();

private:
  Fiber(const Thread&) = delete;
  Fiber& operator =(const Thread&) = delete;

#if ON_WINDOWS
  static VOID CALLBACK FiberProc(_In_ PVOID lpParameter);
#endif;

  FiberFn _fn;
  void* _handle;
  void* _user_data;
  FiberState _state;
};
