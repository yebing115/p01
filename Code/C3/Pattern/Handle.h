#pragma once
#include "Data/DataType.h"

#define INVALID_HANDLE UINT32_MAX

struct Handle {
  u32 age: 16;
  u32 idx: 16;
  Handle() { Reset(); }
  explicit Handle(u32 raw) { *((u32*)this) = raw; }
  operator bool() const { return *((u32*)this) != INVALID_HANDLE; }
  void Reset() { *((u32*)this) = INVALID_HANDLE; }
};
