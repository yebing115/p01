#pragma once
#include "Data/DataType.h"
#include "Data/String.h"
#include "Pattern/Singleton.h"

struct ArchiveDesc {
  const char* _name;
  stringid _name_id;
  u64 _size;
  int _fd;
};

struct FileDesc {
  const char* _name;
  stringid _name_id;
  u32 _c_size;
  u32 _d_size;
  int _archive_fd;
  u64 _archive_offset;
  u16 _file_type;
  u32 _file_flags;
};

class IFile;
class FileSystem {
public:
  FileSystem();
  ~FileSystem();

  IFile* OpenRead(const char* filename) { return Open(filename, false); }
  IFile* OpenWrite(const char* filename) { return Open(filename, true); }
  void Close(IFile* f);
  bool Exists(const char* filename) const;
  vector<String> GetFileList(const String& dir, bool recursive = true);
  const char* GetRootDir() const { return _root_dir; }
  void SetRootDir(const char* dir);
  
private:
  void Init();
  void GetFullPath(const char* filename, char* full_path);
  IFile* Open(const char* filename, bool writable);
  u8* _idx_data;
  vector<ArchiveDesc> _archives;
  vector<const char*> _string_table;
  unordered_map<stringid, FileDesc> _filename_map;
  char _root_dir[1024];
  SUPPORT_SINGLETON(FileSystem);
};
