#include "C3PCH.h"
#include "Logger.h"
#include "Text/EncodingUtil.h"

void StdoutLogger::Log(const char* text) {
  fputs(text, stdout);
}

FileLogger::FileLogger(const String& filename) {
#if ON_IOS
  String path{get_archive_path(), filename};
#else
  const String& path = filename;
#endif
  _file = fopen(path.GetCString(), "wb");
}

FileLogger::~FileLogger() {
  if (_file) fclose(_file);
}

void FileLogger::Log(const char* text) {
  if (_file) {
    fwrite(text, strlen(text), 1, _file);
    fflush(_file);
  }
}

#if ON_WINDOWS
#include "Platform/Windows/WindowsHeader.h"
void VSDebugLogger::Log(const char* text) { OutputDebugString((LPCTSTR)EncodingUtil::UTF8ToSystem(text).GetCString()); }
#endif