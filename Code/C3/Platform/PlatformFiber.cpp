#include "C3PCH.h"
#include "PlatformFiber.h"
#include "Debug/C3Debug.h"

#define FIBER_STACK_SIZE 0

static thread_local Fiber* g_tls_fiber = nullptr;

Fiber::Fiber(): _fn(nullptr), _user_data(nullptr), _state(FIBER_STATE_INITIALIZED) {
  _handle = CreateFiber(FIBER_STACK_SIZE, &Fiber::FiberProc, this);
}

void Fiber::Prepare(FiberFn fn, void* user_data) {
  c3_assert(_state == FIBER_STATE_INITIALIZED || _state == FIBER_STATE_FINISHED);
  _fn = fn;
  _user_data = user_data;
  _state = FIBER_STATE_SUSPENDED;
}

void Fiber::Suspend() {
  c3_assert(_state == FIBER_STATE_RUNNING);
  _state = FIBER_STATE_SUSPENDED;
  if (g_tls_fiber != this) g_tls_fiber->Resume();
}

void Fiber::Finish() {
  c3_assert(_state == FIBER_STATE_RUNNING);
  _state = FIBER_STATE_FINISHED;
  if (g_tls_fiber != this) g_tls_fiber->Resume();
}

void Fiber::Resume() {
  c3_assert(_state == FIBER_STATE_SUSPENDED);
  _state = FIBER_STATE_RUNNING;
  SwitchToFiber(_handle);
}

VOID CALLBACK Fiber::FiberProc(_In_ PVOID lpParameter) {
  auto fiber = (Fiber*)lpParameter;
  do {
    fiber->_fn(fiber->_user_data);
    fiber->Finish();
  } while (1);
}

Fiber* Fiber::ConvertFromThread(void* user_data) {
  if (!g_tls_fiber) {
    g_tls_fiber = new Fiber();
    g_tls_fiber->_user_data = user_data;
    g_tls_fiber->_handle = ConvertThreadToFiber(g_tls_fiber);
    c3_assert(g_tls_fiber);
    g_tls_fiber->_state = FIBER_STATE_RUNNING;
  }
  return g_tls_fiber;
}

Fiber* Fiber::GetScheduleFiber() {
  return g_tls_fiber;
}

void Fiber::SetScheduleFiber(Fiber* fiber) {
  g_tls_fiber = fiber;
}

Fiber* Fiber::GetCurrentFiber() {
  return (Fiber*)::GetFiberData();
}
