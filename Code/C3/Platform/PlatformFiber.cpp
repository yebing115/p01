#include "C3PCH.h"
#include "PlatformFiber.h"
#include "Debug/C3Debug.h"

#define FIBER_STACK_SIZE 0

static thread_local Fiber* tls_fiber = nullptr;

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
  auto sched_fiber = GetThreadMajorFiber();
  if (sched_fiber && sched_fiber != this) sched_fiber->Resume();
}

void Fiber::Finish() {
  c3_assert(_state == FIBER_STATE_RUNNING);
  _state = FIBER_STATE_FINISHED;
  auto sched_fiber = GetThreadMajorFiber();
  if (sched_fiber && sched_fiber != this) sched_fiber->Resume();
}

void Fiber::Resume() {
  c3_assert(_state == FIBER_STATE_SUSPENDED);
  _state = FIBER_STATE_RUNNING;
  SwitchToFiber(_handle);
}

VOID CALLBACK Fiber::FiberProc(_In_ PVOID lpParameter) {
  auto fiber = (Fiber*)lpParameter;
  fiber->_fn(fiber->_user_data);
  fiber->Finish();
}

Fiber* Fiber::ConvertFromThread(void* user_data) {
  if (!tls_fiber) {
    tls_fiber = new Fiber();
    tls_fiber->_user_data = user_data;
    tls_fiber->_handle = ConvertThreadToFiber(tls_fiber);
    c3_assert(tls_fiber);
    tls_fiber->_state = FIBER_STATE_RUNNING;
  }
  return tls_fiber;
}

Fiber* Fiber::GetThreadMajorFiber() {
  return tls_fiber;
}

Fiber* Fiber::GetCurrentFiber() {
  return (Fiber*)::GetFiberData();
}
