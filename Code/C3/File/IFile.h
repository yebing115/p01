#pragma once
#include "Data/DataType.h"
#include "Data/String.h"

class IFile {
public:
  virtual ~IFile() {}
  operator bool() const { return IsValid(); }
  virtual bool IsValid() const { return false; }
  virtual void Close() {}
  virtual int ReadBytes(void* p, int length) { return 0; }
  virtual bool GetLine(void* p, int max_size) { return 0; }
  virtual int WriteBytes(const void* p, int length) { return 0; }
  virtual void Flush() {}
  virtual size_t GetSize() const { return 0; }
  virtual void Seek(i64 offset) {}
  virtual size_t GetOffset() const { return 0; }
};
