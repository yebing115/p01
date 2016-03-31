#include "C3PCH.h"
#include "MemoryBlock.h"

void mem_init() {
  static CrtAllocator s_allocator;
  g_allocator = &s_allocator;
}
