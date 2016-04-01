#pragma once
#include "Memory/Allocator.h"
#include "Data/list.h"
#include "Platform/PlatformSync.h"

template <typename T, size_t NUM>
class ThreadSafePool {
public:
  ThreadSafePool() {
    for (int i = 0; i < NUM - 1; ++i) {
      _free_nodes[i]._next = _free_nodes + i + 1;
    }
    _free_nodes[NUM - 1]._next = nullptr;
    _free_head = _free_nodes;
  }
  ~ThreadSafePool() {}
  T* Get() {
    FreeNode* old_head;
    FreeNode* new_head;
    do {
      old_head = _free_head;
      if (!old_head) return nullptr;
      new_head = old_head->_next;
    } while (!_free_head.compare_exchange_weak(old_head, new_head));
    ++_num_used;
    return &old_head->_object;
  }
  T* GetWait() {
    T* obj;
    do {
      obj = Get();
    } while (!obj);
    return obj;
  }
  void Put(T* obj) {
    FreeNode* new_head = container_of(obj, FreeNode, _object);
    FreeNode* old_head;
    do {
      old_head = _free_head;
      new_head->_next = old_head;
    } while (!_free_head.compare_exchange_weak(old_head, new_head));
    --_num_used;
  }
private:
  struct FreeNode {
    T _object;
    struct FreeNode* _next;
  };
  atomic_size_t _num_used;
  FreeNode _free_nodes[NUM];
  atomic<FreeNode*> _free_head;
};
