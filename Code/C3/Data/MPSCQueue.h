#pragma once
#include "SPSCQueue.h"
#include "Platform/PlatformSync.h"

/*
* MPSCQueue is a multiple producer and one consumer queue
* with producer locks.
*/
template<class T>
struct MPSCQueue {
  typedef T value_type;

  MPSCQueue(const MPSCQueue&) = delete;
  MPSCQueue& operator = (const MPSCQueue&) = delete;

  // size must be >= 2.
  //
  // Also, note that the number of usable slots in the queue at any
  // given time is actually (size-1), so if you start with an empty queue,
  // isFull() will return true after size-1 insertions.
  explicit MPSCQueue(u32 size, IAllocator* allocator = g_allocator)
  : _spsc_queue(size, allocator) {
    _producer_lock.Reset();
  }

  ~MPSCQueue() {}

  template<class ...Args>
  bool Write(Args&&... recordArgs) {
    _producer_lock.Lock();
    bool ok = _spsc_queue.Write(std::forward<Args>(recordArgs)...);
    _producer_lock.Unlock();
    return ok;
  }

  // move (or copy) the value at the front of the queue to given variable
  bool Read(T& record) {
    return _spsc_queue.Read(record);
  }

  // pointer to the value at the front of the queue (for use in-place) or
  // nullptr if empty.
  T* FrontPtr() {
    return _spsc_queue.FrontPtr();
  }

  // queue must not be empty
  void PopFront() {
    _spsc_queue.PopFront();
  }

  bool IsEmpty() const {
    _producer_lock.Lock();
    bool empty = _spsc_queue.IsEmpty();
    _producer_lock.Unlock();
    return empty;
  }

  bool IsFull() const {
    _producer_lock.Lock();
    bool full = _spsc_queue.IsFull();
    _producer_lock.Unlock();
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
  SpinLock _producer_lock;
  SPSCQueue<T> _spsc_queue;
};
