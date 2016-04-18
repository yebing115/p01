#include "C3PCH.h"
#include "Blob.h"

BlobWriter::BlobWriter(IAllocator* allocator): _pos(0) {
  _mem = new AllocatedMemory(allocator);
}

BlobWriter::~BlobWriter() {
  safe_delete(_mem);
}

void BlobWriter::Reserve(int size) {
  if (_mem->size < size) _mem->Resize(size);
}

void BlobWriter::Write(const void* data, int size, void** data_ptr) {
  if (size > 0) {
    if (_pos + size > _mem->size) _mem->Resize(ALIGN_256(_pos + size));
    if (data_ptr) *data_ptr = (u8*)_mem->data + _pos;
    memcpy((u8*)_mem->data + _pos, data, size);
    _pos += size;
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

BlobReader::BlobReader(MemoryRegion* mem)
: _data((const u8*)mem->data), _size(mem->size), _pos(0) {}

BlobReader::BlobReader(const void* data, int size)
: _data((const u8*)data), _size(size), _pos(0) {}

const void* BlobReader::Skip(int size) {
  auto* pos = _data + _pos;
  _pos += size;
  if (_pos > _size) _pos = _size;
  return (const void*)pos;
}

void BlobReader::Seek(u32 pos) {
  _pos = clamp<u32>(pos, 0, _size);
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
  if (!Read(size)) return false;
  if (size > max_size) return false;
  return Read(data, size < max_size ? size : max_size);
}

bool BlobReader::Peek(void* data, u32 size) {
  u32 pos = _pos;
  bool ok = Read(data, size);
  _pos = pos;
  return ok;
}

bool BlobReader::PeekString(char* data, u32 max_size) {
  u32 pos = _pos;
  u32 size;
  if (!Read(size)) {
    _pos = pos;
    return false;
  }
  if (size > max_size) {
    _pos = pos;
    return false;
  }
  bool ok = Read(data, size < max_size ? size : max_size);
  _pos = pos;
  return ok;
}
