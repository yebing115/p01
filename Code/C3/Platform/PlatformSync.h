#pragma once

#include "PlatformConfig.h"
#include "Data/C3Data.h"
#include "Debug/C3Debug.h"
#if ON_WINDOWS
#include "Windows/WindowsHeader.h"
#endif
#include <atomic>
#include <thread>
typedef std::atomic_int32_t atomic_i32;
typedef std::atomic_uint32_t atomic_u32;
typedef std::atomic_int64_t atomic_i64;
typedef std::atomic_uint64_t atomic_u64;
using std::atomic;
using std::atomic_size_t;
using std::atomic_bool;
using std::atomic_int;
using std::atomic_uint;
using std::atomic_long;
using std::atomic_ulong;
using std::atomic_flag;
using std::atomic_exchange;
using std::atomic_compare_exchange_strong;
using std::memory_order_relaxed;
using std::memory_order_consume;
using std::memory_order_acquire;
using std::memory_order_release;
using std::memory_order_acq_rel;
using std::memory_order_seq_cst;

class Semaphore {
public:
  Semaphore() {
#if ON_WINDOWS
    _handle = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
#elif ON_PS4
    auto ret = sceKernelCreateSema(&_handle, "sem", 0, 0, 65535, nullptr);
    c3_assert(ret == SCE_OK);
#endif
  }
  ~Semaphore() {
#if ON_WINDOWS
    CloseHandle(_handle);
#elif ON_PS4
    sceKernelDeleteSema(_handle);
#endif
  }
  void Post(u32 count = 1) const {
#if ON_WINDOWS
    ReleaseSemaphore(_handle, count, NULL);
#elif ON_PS4
    sceKernelSignalSema(_handle, (int)count);
#endif
  }
  bool Wait(i32 msecs = -1) const {
#if ON_WINDOWS
    DWORD milliseconds = (0 > msecs) ? INFINITE : msecs;
    return WAIT_OBJECT_0 == WaitForSingleObject(_handle, milliseconds);
#elif ON_PS4
    if (msecs == -1) return sceKernelWaitSema(_handle, 1, nullptr) == SCE_OK;
    else {
      SceKernelUseconds usecs = msecs * 1000;
      return sceKernelWaitSema(_handle, 1, &usecs) == SCE_OK;
    }
#endif
  }

private:
  Semaphore(const Semaphore&);
  Semaphore& operator =(const Semaphore&);
#if ON_WINDOWS
  HANDLE _handle;
#elif ON_PS4
  SceKernelSema _handle;
#endif
};

class Thread {
public:
  typedef i32(*ThreadFn)(void* user_data);

  Thread()
#if ON_WINDOWS
    : _handle(INVALID_HANDLE_VALUE), _thread_id(UINT32_MAX)
#elif ON_PS4
    : _handle(0), _thread_id(0)
#endif
    , _fn(NULL), _user_data(NULL), _stack_size(0), _exit_code(0), _running(false) {}
  virtual ~Thread() { if (_running) Shutdown(); }

  void Init(ThreadFn fn, void* user_data = NULL, u32 stack_size = 0, const char* name = NULL, int core = -1) {
    c3_assert_return(!_running && "Already running!");

    _fn = fn;
    _user_data = user_data;
    _stack_size = stack_size;
    _running = true;

#if ON_WINDOWS
    _handle = CreateThread(NULL, _stack_size, threadFunc, this, 0, NULL);
#elif ON_PS4
    auto ret = scePthreadCreate(&_handle, nullptr, threadFunc, this, name);
    c3_assert(ret == SCE_OK);
#endif

    _sem.Wait();

    if (name) SetThreadName(name);
    if (core != -1) SetThreadCore(core);
  }

  void Shutdown() {
    c3_assert_return(_running && "Not running!");
#if ON_WINDOWS
    WaitForSingleObject(_handle, INFINITE);
    GetExitCodeThread(_handle, (DWORD*)&_exit_code);
    CloseHandle(_handle);
    _handle = INVALID_HANDLE_VALUE;
    _running = false;
#elif ON_PS4
#endif
  }
  bool IsRunning() const { return _running; }
  i32 GetExitCode() const { return _exit_code; }
  void SetThreadName(const char* _name) {
#if ON_WINDOWS
#  pragma pack(push, 8)
    struct ThreadName {
      DWORD  type;
      LPCSTR name;
      DWORD  id;
      DWORD  flags;
    };
#  pragma pack(pop)
    ThreadName tn;
    tn.type = 0x1000;
    tn.name = _name;
    tn.id = _thread_id;
    tn.flags = 0;

    __try {
      RaiseException(0x406d1388, 0, sizeof(tn) / 4, (ULONG_PTR*)&tn);
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
#elif ON_PS4
#endif
  }
  void SetThreadCore(int core) {
#if ON_WINDOWS
    SetThreadIdealProcessor(_handle, core);
#endif
  }

private:
  Thread(const Thread&);
  Thread& operator =(const Thread&);

  i32 Entry() {
#if ON_WINDOWS
    _thread_id = ::GetCurrentThreadId();
#elif ON_PS4
    _thread_id = scePthreadGetthreadid();
#endif
    _sem.Post();
    return _fn(_user_data);
  }

#if ON_WINDOWS
  static DWORD WINAPI threadFunc(LPVOID arg) {
    Thread* thread = (Thread*)arg;
    i32 result = thread->Entry();
    return result;
  }
  HANDLE _handle;
  DWORD  _thread_id;

#elif ON_PS4
  static void* threadFunc(void* arg) {
    Thread* thread = (Thread*)arg;
    i32 result = thread->Entry();
    return (void*)ptrdiff_t(result);
  }
  ScePthread _handle;
  int _thread_id;
#endif

  ThreadFn _fn;
  void* _user_data;
  Semaphore _sem;
  u32 _stack_size;
  i32 _exit_code;
  bool _running;
};

class TlsData {
public:
  TlsData() {
#if ON_WINDOWS
    _id = TlsAlloc();
    c3_assert_return(TLS_OUT_OF_INDEXES != _id && "Failed to allocated TLS index.");
#endif
  }

  ~TlsData() {
#if ON_WINDOWS
    BOOL result = TlsFree(_id);
    c3_assert_return(result && "Failed to free TLS index.");
#endif
  }

  void* Get() const {
#if ON_WINDOWS
    return TlsGetValue(_id);
#endif
    return nullptr;
  }
  void Set(void* ptr) {
#if ON_WINDOWS
    TlsSetValue(_id, ptr);
#endif
  }

private:
  u32 _id;
};

struct SpinLock {
  atomic_flag _flag;

  void Reset() { _flag.clear(); }
  void Lock() { while (_flag.test_and_set(memory_order_acquire)); }
  void Unlock() { _flag.clear(memory_order_release); }
  bool TryLock() { return !_flag.test_and_set(memory_order_acquire); }
};