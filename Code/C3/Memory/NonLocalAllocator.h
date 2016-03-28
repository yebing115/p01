#pragma once

#include "Data/DataType.h"

// First-fit
class NonLocalAllocator {
public:
  NonLocalAllocator() {}
  ~NonLocalAllocator() {}

  void Reset() {
    _free.clear();
    _used.clear();
  }

  void Add(u64 _ptr, u32 _size) {
    _free.push_back(FreeBlock(_ptr, _size));
  }

  u64 Remove() {
    if (!_free.empty()) {
      FreeBlock free_block = _free.front();
      _free.pop_front();
      return free_block._ptr;
    }
    return 0;
  }

  u64 Alloc(u32 size) {
    for (FreeList::iterator it = _free.begin(), it_end = _free.end(); it != it_end; ++it) {
      if (it->_size >= size) {
        u64 ptr = it->_ptr;

        _used.insert(make_pair(ptr, size));

        if (it->_size != size) {
          it->_size -= size;
          it->_ptr += size;
        } else {
          _free.erase(it);
        }

        return ptr;
      }
    }

    // there is no block large enough.
    return UINT64_MAX;
  }

  void Free(u64 block) {
    UsedList::iterator it = _used.find(block);
    if (it != _used.end()) {
      _free.push_front(FreeBlock(it->first, it->second));
      _used.erase(it);
    }
  }

  bool Compact() {
    _free.sort();

    for (FreeList::iterator it = _free.begin(), next = it, it_end = _free.end(); next != it_end;) {
      if ((it->_ptr + it->_size) == next->_ptr) {
        it->_size += next->_size;
        next = _free.erase(next);
      } else {
        it = next;
        ++next;
      }
    }

    return 0 == _used.size();
  }

private:
  struct FreeBlock {
    u64 _ptr;
    u32 _size;

    FreeBlock(u64 ptr, u32 size): _ptr(ptr), _size(size) {}
    bool operator<(const FreeBlock& rhs) const { return _ptr < rhs._ptr; }
  };

  typedef list<FreeBlock> FreeList;
  FreeList _free;

  typedef unordered_map<u64, u32> UsedList;
  UsedList _used;
};
