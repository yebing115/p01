#pragma once
#include "IFile.h"

struct FileDesc;
class Lz4File : public IFile {
public:
  Lz4File(const FileDesc* desc);
  bool IsValid() const override { return true; }
  void Close() override;
  int ReadBytes(void* p, int length) override;
  bool GetLine(void* p, int max_size) override;
  size_t GetSize() const override { return _size; }
  void Seek(i64 offset) override;
  size_t GetOffset() const  override{ return (size_t)_offset; }
private:
  bool PullChunk();
  const FileDesc* _desc;
  u8* _buf;
  u8* _buf_i;
  u8* _buf_z;
  u64 _archive_offset;
  u64 _offset;
  u32 _size;
};
