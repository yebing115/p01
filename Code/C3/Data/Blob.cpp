#include "Blob.h"

BlobWriter::BlobWriter() {}
BlobWriter::BlobWriter(const BlobWriter& rhs) : _data(rhs._data) {}

BlobWriter& BlobWriter::operator =(const BlobWriter& rhs) {
  _data = rhs._data;
  return *this;
}

void BlobWriter::Write(const void* data, int size) {
  if (size) {
    int pos = _data.size();
    _data.resize(_data.size() + size);
    memcpy(&_data[0] + pos, data, size);
  }
}

void BlobWriter::WriteString(const char* string) {
  if (string) {
    u32 size = strlen(string) + 1;
    Write(size);
    Write(string, size);
  } else {
    Write((u32)0);
  }
}

BlobReader::BlobReader(const void* data, int size)
: _data((const u8*)data), _size(size), _pos(0) {}

BlobReader::BlobReader(const BlobWriter& blob)
: _data((const u8*)blob.GetData()), _size(blob.GetSize()), _pos(0) {}

const void* BlobReader::Skip(int size) {
  auto* pos = _data + _pos;
  _pos += size;
  if (_pos > _size) _pos = _size;
  return (const void*)pos;
}

bool BlobReader::Read(void* data, u32 size) {
  if (_pos + (int)size > _size) {
    for (u32 i = 0; i < size; ++i) ((unsigned char*)data)[i] = 0;
    return false;
  }
  if (size) memcpy(data, ((char*)_data) + _pos, size);
  _pos += size;
  return true;
}

bool BlobReader::ReadString(char* data, u32 max_size) {
  u32 size;
  Read(size);
  if (size > max_size) return false;
  return Read(data, size < max_size ? size : max_size);
}
