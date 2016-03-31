#pragma once

#pragma once
#include "SPSCQueue.h"
#include "Platform/PlatformSync.h"

/*
* MPMCQueue is a multiple producer and multiple consumer queue
* with lock.
*/
template<class T>
struct MPMCQueue {
  typedef T value_type;

  MPMCQueue(const MPMCQueue&) = delete;
  MPMCQueue& operator = (const MPMCQueue&) = delete;

  // size must be >= 2.
  //
  // Also, note that the number of usable slots in the queue at any
  // given time is actually (size-1), so if you start with an empty queue,
  // isFull() will return true after size-1 insertions.
  explicit MPMCQueue(u32 size, IAllocator* allocator = g_allocator)
  : _spsc_queue(size, allocator) {
    _lock.Reset();
  }

  ~MPMCQueue() {}

  template<class ...Args>
  bool Write(Args&&... recordArgs) {
    _lock.Lock();
    bool ok = _spsc_queue.Write(std::forward<Args>(recordArgs)...);
    _lock.Unlock();
    return ok;
  }

  // move (or copy) the value at the front of the queue to given variable
  bool Read(T& record) {
    _lock.Lock();
    bool ok = _spsc_queue.Read(record);
    _lock.Unlock();
    return ok;
  }

  // pointer to the value at the front of the queue (for use in-place) or
  // nullptr if empty.
  T* FrontPtr() {
    _lock.Lock();
    T* ptr = _spsc_queue.FrontPtr();
    _lock.Unlock();
    return ptr;
  }

  // queue must not be empty
  void PopFront() {
    _lock.Lock();
    _spsc_queue.PopFront();
    _lock.Unlock();
  }

  bool IsEmpty() const {
    _lock.Lock();
    bool empty = _spsc_queue.IsEmpty();
    _lock.Unlock();
    return empty;
  }

  bool IsFull() const {
    _lock.Lock();
    bool full = _spsc_queue.IsFull();
    _lock.Unlock();
    return full;
  }

  // * If called by consumer, then true size may be more (because producer may
  //   be adding items concurrently).
  // * If called by producer, then true size may be less (because consumer may
  //   be removing items concurrently).
  // * It is undefined to call this from any other thread.
  size_t SizeGuess() const {
    return _spsc_queue.SizeGuess();
  }

private:
  SpinLock _lock;
  SPSCQueue<T> _spsc_queue;
};


