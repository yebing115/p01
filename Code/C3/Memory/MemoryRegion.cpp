#include "C3PCH.h"
#include "MemoryRegion.h"

void mem_init() {
  static CrtAllocator s_allocator;
  g_allocator = &s_allocator;
}
