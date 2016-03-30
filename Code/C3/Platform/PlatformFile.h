#pragma once
#include "Data/DataType.h"
#include "Data/String.h"

extern int platform_fopen_read(const char* fname);
extern int platform_fclose(int fd);
extern int platform_pread(int fd, void* buf, size_t size, u64 offset);
extern int platform_mkdir(const char* dir_path);
extern vector<String> platform_get_file_list(const String& path, bool recursive);