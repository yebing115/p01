#include "CrtFile.h"
#include "Text/EncodingUtil.h"
#include <sys/stat.h>

#if ON_WINDOWS
#pragma warning(disable:4996)
#endif

CrtFile::CrtFile(const String& fname, bool writable): _file(nullptr) {
  auto platform_filename = EncodingUtil::UTF8ToSystem(fname);
  _file = fopen(platform_filename.GetCString(), writable ? "wb" : "rb");
  _file_size = 0;
  if (_file) {
    struct stat st;
    if (stat(platform_filename.GetCString(), &st) == 0) _file_size = st.st_size;
  }
}

void CrtFile::Close() {
  if (_file) {
    auto ok = fclose(_file);
    _file = nullptr;
  }
}

int CrtFile::ReadBytes(void* p, int length) {
  return fread(p, 1, length, _file);
}

bool CrtFile::GetLine(void* p, int max_size) {
  return fgets((char*)p, max_size, _file) != nullptr;
}

int CrtFile::WriteBytes(const void* p, int length) {
  return fwrite(p, 1, length, _file);
}

void CrtFile::Flush() {
  fflush(_file);
}

size_t CrtFile::GetSize() const {
  return _file_size;
}

void CrtFile::Seek(i64 offset) {
  fseek(_file, (long)offset, SEEK_SET);
}

size_t CrtFile::GetOffset() const {
  return ftell(_file);
}
