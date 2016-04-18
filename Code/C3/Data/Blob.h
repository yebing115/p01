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
  int GetCapacity() const { return (int)_mem->size; }
  void Write(const void* data, int size, void** data_ptr = nullptr);
  void WriteString(const char* string);
  template <class T> void Write(const T& value, void** data_ptr = nullptr) {
    Write(&value, sizeof(T), data_ptr);
  }
  template <> void Write<bool>(const bool& value, void** data_ptr) {
    u32 v = value;
    Write(&v, sizeof(v));
  }
  
  u32 GetPos() const { return _pos; }
  void Skip(u32 n) {
    if (n > 0) {
      Reserve(_pos + n);
      memset((u8*)_mem->data + _pos, 0, n);
      _pos += n;
    }
  }
  void Seek(u32 pos) {
    if (_pos <= pos) _pos = pos;
    else Skip(pos - _pos);
  }
  void Reset() { _pos = 0; }

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
    u32 v;
    Read(&v, sizeof(v));
    return v != 0;
  }
  const void* Skip(int size);
  const void* GetData() const { return (const void*)_data; }
  u32 GetSize() const { return _size; }
  void SetPos(int pos) { _pos = pos; }
  u32 GetPos() const { return _pos; }
  void Reset() { _pos = 0; }
  void Skip(u32 n) { Seek(_pos + n); }
  void Seek(u32 pos);

private:
  const u8* _data;
  u32 _size;
  u32 _pos;

  NON_COPYABLE(BlobReader);
};
