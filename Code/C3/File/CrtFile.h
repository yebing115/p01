#pragma once
#include "IFile.h"

class CrtFile: public IFile {
public:
  CrtFile(const String& fname, bool writable = false);
  
  bool IsValid() const override { return _file != nullptr; }
  int ReadBytes(void* p, int length) override;
  bool GetLine(void* p, int max_size) override;
  int WriteBytes(const void* p, int length) override;
  void Flush() override;
  size_t GetSize() const override;
  void Seek(i64 offset) override;
  size_t GetOffset() const override;
  void Close() override;
private:
  FILE* _file;
  size_t _file_size;
  friend class FileSystem;
};
