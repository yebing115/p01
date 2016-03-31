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
  typedef void(*FiberFinishFn)();
  // INITIALIZED -> SUSPENDED
  void Prepare(FiberFn fn, void* data, FiberFinishFn finish_fn = nullptr);
  // RUNNING -> SUSPENDED
  void Suspend();
  // RUNNING -> FINISHED
  void Finish();
  // SUSPENDED -> RUNNING
  void Resume();
  void* GetData() const { return _user_data; }

  static Fiber* ConvertFromThread(void* user_data);
  static Fiber* GetThreadMajorFiber();
  static Fiber* GetCurrentFiber();

private:
  Fiber(const Thread&) = delete;
  Fiber& operator =(const Thread&) = delete;

#if ON_WINDOWS
  static VOID CALLBACK FiberProc(_In_ PVOID lpParameter);
#endif;

  FiberFn _fn;
  FiberFinishFn _finish_fn;
  void* _handle;
  void* _user_data;
  FiberState _state;
};
