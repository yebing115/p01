#include "C3PCH.h"
#include "Platform/PlatformFile.h"
#include "Data/DataType.h"
#include "Data/String.h"
#include "Text/EncodingUtil.h"
#include "WindowsHeader.h"
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <tchar.h>

int platform_fopen_read(const char* fname) {
  int fd = -1;
  _sopen_s(&fd, fname, _O_RDONLY | _O_BINARY, _SH_DENYRD, _S_IREAD);
  return fd;
}

int platform_fclose(int fd) {
  return _close(fd);
}

int platform_mkdir(const char* dir) {
  return _mkdir(dir);
}

struct CriticalSectionBlock {
  CRITICAL_SECTION _cs;

  CriticalSectionBlock() {
    InitializeCriticalSection(&_cs);
    EnterCriticalSection(&_cs);
  }
  ~CriticalSectionBlock() {
    LeaveCriticalSection(&_cs);
    DeleteCriticalSection(&_cs);
  }
};

int platform_pread(int fd, void* buf, size_t size, u64 offset) {
  CriticalSectionBlock block;
  _lseek(fd, (long)offset, SEEK_SET);
  return _read(fd, buf, size);
}

static void s_get_file_list_append(const String& path_system, const String& prefix_system, bool recursive, vector<String>* list_out) {
  WIN32_FIND_DATA find_data;
  HANDLE handle = FindFirstFile((LPCTSTR)(path_system + "*").GetCString(), &find_data);
  if (handle == INVALID_HANDLE_VALUE) return;
  do {
    if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      list_out->emplace_back(EncodingUtil::SystemToUTF8(prefix_system + find_data.cFileName));
    } else if (recursive) {
      auto dir_name = find_data.cFileName;
      if (_tcscmp(dir_name, _T(".")) && _tcscmp(dir_name, _T(".."))) {
        s_get_file_list_append(path_system + dir_name + "/", prefix_system + dir_name + "/", true, list_out);
      }
    }
  } while (FindNextFile(handle, &find_data));
}

vector<String> platform_get_file_list(const String& path, bool recursive) {
  vector<String> list;
  s_get_file_list_append(EncodingUtil::UTF8ToSystem(path + "/"), String(), recursive, &list);
  return list;
}
