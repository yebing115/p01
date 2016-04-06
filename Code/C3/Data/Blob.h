#pragma once
#include "DataType.h"
#include "Pattern/NonCopyable.h"
#include "Memory/MemoryRegion.h"

class BlobWriter {
public:
  BlobWriter(IAllocator* allocator = g_allocator);
  ~BlobWriter();

  void Reserve(int size);
  const void* GetData() const { return _mem->data; }
  int GetSize() const { return (int)_mem->size; }
  void Write(const void* data, int size);
  void WriteString(const char* string);
  template <class T> void Write(const T& value) { Write(&value, sizeof(T)); }
  template <> void Write<bool>(const bool& value) {
    u8 v = value;
    Write(&v, sizeof(v));
  }
  void Clear() { _pos = 0; }

private:
  AllocatedMemory* _mem;
  u32 _pos;

  NON_COPYABLE(BlobWriter);
};

class BlobReader {
public:
  BlobReader(MemoryRegion* mem);
  BlobReader(const void* data, int size);

  bool Read(void* data, u32 size);
  bool ReadString(char* data, u32 max_size);
  template <class T> void Read(T& value) { Read(&value, sizeof(T)); }
  template <class T> T Read() {
    T v;
    Read(&v, sizeof(v));
    return v;
  }
  template <> bool Read<bool>() {
    u8 v;
    Read(&v, sizeof(v));
    return v != 0;
  }
  const void* Skip(int size);
  const void* GetData() const { return (const void*)_data; }
  int GetSize() const { return _size; }
  void SetPosition(int pos) { _pos = pos; }
  int GetPosition() const { return _pos; }
  void Rewind() { _pos = 0; }

private:
  const u8* _data;
  int _size;
  int _pos;

  NON_COPYABLE(BlobReader);
};
