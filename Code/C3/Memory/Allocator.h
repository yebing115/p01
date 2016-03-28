#pragma once
#include "Data/DataType.h"
#include <memory.h>

#define ALIGN_MASK(value, mask) (((value) + (mask)) & ((~0) & (~(mask))))
#define ALIGN_16(value) ALIGN_MASK(value, 0xf)
#define ALIGN_256(value) ALIGN_MASK(value, 0xff)
#define ALIGN_4096(value) ALIGN_MASK(value, 0xfff)

#ifdef C3_MEMORY_DEBUG
#define C3_ALLOC(allocator, size)  c3_alloc(allocator, size, 0, __FILE__, __LINE__)
#define C3_FREE(allocator, ptr)  c3_free(allocator, ptr, 0, __FILE__, __LINE__)
#define C3_REALLOC(allocator, ptr, size)  c3_realloc(allocator, ptr, size, 0, __FILE__, __LINE__)
#define C3_ALIGNED_ALLOC(allocator, size, align)  c3_alloc(allocator, size, align, __FILE__, __LINE__)
#define C3_ALIGNED_REALLOC(allocator, ptr, size, align) c3_realloc(allocator, ptr, size, align, __FILE__, __LINE__)
#define C3_ALIGNED_FREE(allocator, ptr, align)  c3_free(allocator, ptr, align, __FILE__, __LINE__)
#define C3_NEW(allocator, type) ::new(C3_ALLOC(allocator, sizeof(type))) type
#define C3_DELETE(allocator, ptr) c3_delete_object(allocator, ptr, 0, __FILE__, __LINE__)
#define C3_ALIGNED_NEW(allocator, type, align) ::new(C3_ALIGNED_ALLOC(allocator, sizeof(type), align)) type
#define C3_ALIGNED_DELETE(allocator, ptr, align) c3_delete_object(allocator, ptr, align, __FILE__, __LINE__)
#else
#define C3_ALLOC(allocator, size)  c3_alloc(allocator, size, 0)
#define C3_FREE(allocator, ptr)  c3_free(allocator, ptr, 0)
#define C3_REALLOC(allocator, ptr, size)  c3_realloc(allocator, ptr, size, 0)
#define C3_ALIGNED_ALLOC(allocator, size, align)  c3_alloc(allocator, size, align)
#define C3_ALIGNED_REALLOC(allocator, ptr, size, align) c3_realloc(allocator, ptr, size, align)
#define C3_ALIGNED_FREE(allocator, ptr, align)  c3_free(allocator, ptr, align)
#define C3_NEW(allocator, type) ::new(C3_ALLOC(allocator, sizeof(type))) type
#define C3_DELETE(allocator, ptr) c3_delete(allocator, ptr, 0)
#define C3_ALIGNED_NEW(allocator, type, align) ::new(C3_ALIGNED_ALLOC(allocator, sizeof(type), align)) type
#define C3_ALIGNED_DELETE(allocator, ptr, align) c3_delete(allocator, ptr, align)
#endif

struct Allocator {
  virtual ~Allocator() = 0;
  virtual void* Alloc(size_t size, size_t align, const char* file, u32 line) = 0;
  virtual void Free(void* ptr, size_t align, const char* file, u32 line) = 0;
  virtual void* Realloc(void* ptr, size_t size, size_t align, const char* file, u32 line) = 0;
};
inline Allocator::~Allocator() {}

extern Allocator* g_allocator;

inline void* align_ptr(void* ptr, size_t extra, size_t align = sizeof(ptrdiff_t)) {
	union { void* ptr; size_t addr; } un;
	un.ptr = ptr;
	size_t unaligned = un.addr + extra; // space for header
	size_t mask = align - 1;
	size_t aligned = ALIGN_MASK(unaligned, mask);
	un.addr = aligned;
	return un.ptr;
}

inline void* c3_alloc(Allocator* allocator, size_t size, size_t align = 0, const char* file = nullptr, u32 line = 0) {
	return allocator->Alloc(size, align, file, line);
}

inline void c3_free(Allocator* allocator, void* ptr, size_t align = 0, const char* file = nullptr, u32 line = 0) {
	allocator->Free(ptr, align, file, line);
}

inline void* c3_realloc(Allocator* allocator, void* ptr, size_t size, size_t align = 0, const char* file = nullptr, u32 line = 0) {
	return allocator->Realloc(ptr, size, align, file, line);
}

static inline void* c3_aligned_alloc(Allocator* allocator, size_t size, size_t align, const char* file = nullptr, u32 line = 0) {
	size_t total = size + align;
	u8* ptr = (u8*)c3_alloc(allocator, total, 0, file, line);
	u8* aligned = (u8*)align_ptr(ptr, sizeof(u32), align);
	u32* header = (u32*)aligned - 1;
	*header = u32(aligned - ptr);
	return aligned;
}

static inline void c3_aligned_free(Allocator* allocator, void* ptr_, size_t /*align*/, const char* file = nullptr, u32 line = 0) {
	u8* aligned = (u8*)ptr_;
	u32* header = (u32*)aligned - 1;
	u8* ptr = aligned - *header;
	c3_free(allocator, ptr, 0, file, line);
}

static inline void* c3_aligned_realloc(Allocator* allocator, void* ptr_, size_t size, size_t align, const char* file = nullptr, u32 line = 0) {
	if (!ptr_) return c3_aligned_alloc(allocator, size, align, file, line);

	u8* aligned = (u8*)ptr_;
	u32 offset = *( (u32*)aligned - 1);
	u8* ptr = aligned - offset;
	size_t total = size + align;
	ptr = (u8*)c3_realloc(allocator, ptr, total, 0, file, line);
	u8* new_aligned = (u8*)align_ptr(ptr, sizeof(u32), align);

	if (new_aligned == aligned) return aligned;

	aligned = ptr + offset;
	::memmove(new_aligned, aligned, size);
	u32* header = (u32*)new_aligned - 1;
	*header = u32(new_aligned - ptr);
	return new_aligned;
}

template <typename T>
inline void c3_delete(Allocator* allocator, T* object, size_t align = 0, const char* file = nullptr, u32 line = 0) {
	if (object) {
		object->~T();
		c3_free(allocator, object, align, file, line);
	}
}

struct CrtAllocator: public Allocator {
  ~CrtAllocator() {}
  void* Alloc(size_t size, size_t align, const char* /*file*/, u32 line) override {
    if (align <= sizeof(ptrdiff_t)) return ::malloc(size);
    else {
#if ON_WINDOWS
      return _aligned_malloc(size, align);
#else
      return c3_aligned_alloc(this, size, align, file, line);
#endif
    }
  }
  void Free(void* ptr, size_t align, const char* file, u32 line) override {
    if (align <= sizeof(ptrdiff_t)) return ::free(ptr);
    else {
#if ON_WINDOWS
      return _aligned_free(ptr);
#else
      return c3_aligned_free(this, ptr, align, file, line);
#endif
    }
  }
  void* Realloc(void* ptr, size_t size, size_t align, const char* file, u32 line) override {
    if (align <= sizeof(ptrdiff_t)) return ::realloc(ptr, size);
    else {
#if ON_WINDOWS
      return _aligned_realloc(ptr, size, align);
#else
      return c3_aligned_realloc(this, ptr, size, align, file, line);
#endif
    }
  }
};
