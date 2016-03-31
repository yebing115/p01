//
//  SPSCQueue.h
//
// Stolen from Facebook Folly library.
//   https://github.com/facebook/folly/blob/master/folly/ProducerConsumerQueue.h
//
#pragma once
#include "Data/DataType.h"
#include "Memory/Allocator.h"
#include "Platform/PlatformSync.h"
#include <cstdlib>
#include <stdexcept>
#include <type_traits>
#include <utility>

/*
 * SPSCQueue is a one producer and one consumer queue
 * without locks.
 */
template<class T>
struct SPSCQueue {
  typedef T value_type;

  SPSCQueue(const SPSCQueue&) = delete;
  SPSCQueue& operator = (const SPSCQueue&) = delete;

  // size must be >= 2.
  //
  // Also, note that the number of usable slots in the queue at any
  // given time is actually (size-1), so if you start with an empty queue,
  // isFull() will return true after size-1 insertions.
  explicit SPSCQueue(u32 size, IAllocator* allocator = g_allocator)
  : _size(size)
  , _allocator(allocator)
  , _records(static_cast<T*>(C3_ALLOC(allocator, sizeof(T) * size)))
  , _read_index(0)
  , _write_index(0) {
    c3_assert(size >= 2);
    c3_assert(_records);
  }

  ~SPSCQueue() {
    // We need to destruct anything that may still exist in our queue.
    // (No real synchronization needed at destructor time: only one
    // thread can be doing this.)
    if (!std::is_trivially_destructible<T>::value) {
      size_t read = _read_index;
      size_t end = _write_index;
      while (read != end) {
        _records[read].~T();
        if (++read == _size) {
          read = 0;
        }
      }
    }

    C3_FREE(_allocator, _records);
  }

  template<class ...Args>
  bool Write(Args&&... recordArgs) {
    auto const currentWrite = _write_index.load(std::memory_order_relaxed);
    auto nextRecord = currentWrite + 1;
    if (nextRecord == _size) {
      nextRecord = 0;
    }
    if (nextRecord != _read_index.load(std::memory_order_acquire)) {
      new (&_records[currentWrite]) T(std::forward<Args>(recordArgs)...);
      _write_index.store(nextRecord, std::memory_order_release);
      return true;
    }

    // queue is full
    return false;
  }

  // move (or copy) the value at the front of the queue to given variable
  bool Read(T& record) {
    auto const currentRead = _read_index.load(std::memory_order_relaxed);
    if (currentRead == _write_index.load(std::memory_order_acquire)) {
      // queue is empty
      return false;
    }

    auto nextRecord = currentRead + 1;
    if (nextRecord == _size) {
      nextRecord = 0;
    }
    record = std::move(_records[currentRead]);
    _records[currentRead].~T();
    _read_index.store(nextRecord, std::memory_order_release);
    return true;
  }

  // pointer to the value at the front of the queue (for use in-place) or
  // nullptr if empty.
  T* FrontPtr() {
    auto const currentRead = _read_index.load(std::memory_order_relaxed);
    if (currentRead == _write_index.load(std::memory_order_acquire)) {
      // queue is empty
      return nullptr;
    }
    return &_records[currentRead];
  }

  // queue must not be empty
  void PopFront() {
    auto const currentRead = _read_index.load(std::memory_order_relaxed);
    c3_assert(currentRead != _write_index.load(std::memory_order_acquire));

    auto nextRecord = currentRead + 1;
    if (nextRecord == _size) {
      nextRecord = 0;
    }
    _records[currentRead].~T();
    _read_index.store(nextRecord, std::memory_order_release);
  }

  bool IsEmpty() const {
    return _read_index.load(std::memory_order_consume) ==
      _write_index.load(std::memory_order_consume);
  }

  bool IsFull() const {
    auto nextRecord = _write_index.load(std::memory_order_consume) + 1;
    if (nextRecord == _size) {
      nextRecord = 0;
    }
    if (nextRecord != _read_index.load(std::memory_order_consume)) {
      return false;
    }
    // queue is full
    return true;
  }

  // * If called by consumer, then true size may be more (because producer may
  //   be adding items concurrently).
  // * If called by producer, then true size may be less (because consumer may
  //   be removing items concurrently).
  // * It is undefined to call this from any other thread.
  size_t SizeGuess() const {
    int ret = _write_index.load(std::memory_order_consume) -
      _read_index.load(std::memory_order_consume);
    if (ret < 0) {
      ret += _size;
    }
    return ret;
  }

private:
  const u32 _size;
  IAllocator* _allocator;
  T* const _records;

  atomic_uint _read_index;
  atomic_uint _write_index;
};
