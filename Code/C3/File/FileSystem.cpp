#include "C3PCH.h"
#include "FileSystem.h"
#include "Platform/C3Platform.h"
#include "IFile.h"
#include "Lz4File.h"
#include "CrtFile.h"
#include "Memory/C3Memory.h"
#include "Text/EncodingUtil.h"
#include <stdio.h>
#if ON_PS4
#include "platform/ps4/sce_headers.h"
#endif

DEFINE_SINGLETON_INSTANCE(FileSystem);

#ifdef c3_log
#undef c3_log
#define c3_log(...) printf(__VA_ARGS__)
#endif

#pragma pack(push, 1)
struct ManifestDescRecord {
  u32 _magic;
  u32 _revision;
  u32 _num_archives;
  u32 _num_files;
  u32 _stab_offset;
  u32 _stab_size;
};

struct ArchiveDescRecord {
  u32 _name_id;
  u32 _num_files;
  u64 _size;
};

struct FileDescRecord {
  u32 _name_id;
  u32 _archive_name_id;
  u64 _offset;
  u32 _c_size;
  u32 _d_size;
  u16 _archive_type;
  u16 _file_type;
  u32 _file_flags;
};
#pragma pack(pop)

FileSystem::FileSystem() {
  Init();
}

FileSystem::~FileSystem() {
}

IFile* FileSystem::Open(const String& filename, bool writable) {
  IFile* f = nullptr;
  if (g_platform_data.use_archive) {
    auto it = _filename_map.find(filename.GetID());
    if (it == _filename_map.end()) {
      f = new CrtFile(filename, writable);
    } else f = new Lz4File(&it->second);
  } else {
    f = new CrtFile(filename, writable);
  }
  if (f && !f->IsValid()) safe_delete(f);
  return f;
}

void FileSystem::Close(IFile* f) {
  if (f) {
    f->Close();
    delete f;
  }
}

bool FileSystem::Exists(const String& filename) const {
  if (g_platform_data.use_archive) {
    for (const char* s : _string_table) {
      if (strcmp(filename.GetCString(), s) == 0) return true;
    }
  } else {
    FILE* f = nullptr;
    auto platform_filename = EncodingUtil::UTF8ToSystem(filename);
    f = fopen(platform_filename.GetCString(), "r");
    bool exist = (f != nullptr);
    if (f) {
      fclose(f);
      return true;
    }
  }
  return false;
}

vector<String> FileSystem::GetFileList(const String& dir_, bool recursive) {
  auto dir = dir_.CanonicalPath();
  if (g_platform_data.use_archive) {
    vector<String> l;
    for (const char* s : _string_table) {
      auto len = dir.GetLength();
      if (strncmp(dir.GetCString(), s, len) == 0 && s[len] == '/') {
        if (recursive || !strchr(s + len + 1, '/')) l.push_back(s + len + 1);
      }
    }
    return l;
  } else {
    return platform_get_file_list(dir, recursive);
  }
}

static inline const char* next_string(const char* p) {
  return p + strlen(p) + 1;
}

void FileSystem::Init() {
  if (g_platform_data.use_archive) {
    FILE* f = nullptr;
#if ON_PS4
    fopen(&f, "/app0/GameResources/resource.idx", "rb");
#else
    f = fopen("resource.idx", "rb");
#endif
    if (!f) {
      c3_log("[C2] Failed to load resource.idx.\n");
      abort();
    }
    fseek(f, 0, SEEK_END);
    u32 size = (u32)ftell(f);
    fseek(f, 0, SEEK_SET);
    _idx_data = (u8*)C3_ALLOC(g_allocator, size);
    fread(_idx_data, size, 1, f);
    fclose(f);
    ManifestDescRecord* manifest = (ManifestDescRecord*)_idx_data;
    _archives.reserve(manifest->_num_archives);
    _filename_map.reserve(manifest->_num_files);
    _string_table.reserve(manifest->_num_archives + manifest->_num_files);
    ArchiveDescRecord* archive = (ArchiveDescRecord*)&manifest[1];
    const char* p = (const char*)_idx_data + manifest->_stab_offset;
    char archive_path[256];
    for (u32 i = 0; i < manifest->_num_archives; ++i, ++archive) {
      ArchiveDesc ar;
      ar._name = p;
      _string_table.push_back(p);
      p = next_string(p);
      ar._name_id = archive->_name_id;
      ar._size = archive->_size;
#if ON_PS4
      snprintf(archive_path, sizeof(archive_path), "/app0/GameResources/%s", ar._name);
#else
      snprintf(archive_path, sizeof(archive_path), "%s", ar._name);
#endif
      ar._fd = platform_fopen_read(archive_path);
      if (ar._fd < 0) {
        c3_log("[C2] open %s failed, error: %d.\n", ar._name, ar._fd);
        abort();
      }
      _archives.push_back(ar);
    }
    FileDescRecord* frec = (FileDescRecord*)archive;
    for (u32 i = 0; i < manifest->_num_files; ++i, ++frec) {
      FileDesc& desc = _filename_map[frec->_name_id];
      desc._name = p;
      _string_table.push_back(p);
      p = next_string(p);
      desc._name_id = frec->_name_id;
      desc._c_size = frec->_c_size;
      desc._d_size = frec->_d_size;
      desc._file_type = frec->_file_type;
      desc._file_flags = frec->_file_flags;
      desc._archive_fd = -1;
      desc._archive_offset = 0;
      for (auto& ar : _archives) {
        if (ar._name_id == frec->_archive_name_id) {
          desc._archive_fd = ar._fd;
          desc._archive_offset = frec->_offset;
          break;
        }
      }
      if (desc._archive_fd == -1) {
        c3_log("[C2] Failed to lookup '%s' in archive.\n", desc._name);
        abort();
      }
    }
  }
}
