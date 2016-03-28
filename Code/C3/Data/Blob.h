#pragma once
#include "DataType.h"

class BlobWriter {
public:
  BlobWriter();
  BlobWriter(const BlobWriter& blob);
  BlobWriter& operator =(const BlobWriter& rhs);

  void Reserve(int size) { _data.reserve(size); }
  const void* GetData() const { return _data.empty() ? nullptr : &_data[0]; }
  int GetSize() const { return _data.size(); }
  void Write(const void* data, int size);
  void WriteString(const char* string);
  template <class T> void Write(const T& value) { Write(&value, sizeof(T)); }
  template <> void Write<bool>(const bool& value) {
    u8 v = value;
    Write(&v, sizeof(v));
  }
  void Clear() { _data.clear(); }

private:
  vector<u8> _data;
};

class BlobReader {
public:
  BlobReader(const void* data, int size);
  explicit BlobReader(const BlobWriter& blob);

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
};
