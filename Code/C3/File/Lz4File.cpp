#include "C3PCH.h"
#include "Lz4File.h"
#include "FileSystem.h"
#include "Algorithm/C3Algorithm.h"
#include "Debug/C3Debug.h"
#include "Memory/C3Memory.h"
#include <lz4.h>

#define CHUNK_SIZE 0x10000
#define CPTR (_buf + CHUNK_SIZE)
const u32 FILE_MAGIC = 0x7f7f5a5a;
const u32 NC_MASK = 0x80000000;

Lz4File::Lz4File(const FileDesc* desc): _desc(desc), _archive_offset(_desc->_archive_offset), _offset(0), _size(desc->_d_size) {
  _buf = (u8*)C3_ALIGNED_ALLOC(g_allocator, CHUNK_SIZE * 2, CACHELINE_SIZE);
  _buf_i = _buf;
  _buf_z = _buf;
  platform_pread(_desc->_archive_fd, CPTR, 4, _archive_offset);
  _archive_offset += 4;
  c3_assert(*(u32*)CPTR == FILE_MAGIC);
  PullChunk();
}

void Lz4File::Close() {
  C3_ALIGNED_FREE(g_allocator, _buf, CACHELINE_SIZE);
}

int Lz4File::ReadBytes(void* p, int length) {
  auto p_i = (u8*)p;
  auto p_z = (u8*)p + length;
  while (p_i < p_z) {
    if (_buf_i == _buf_z) {
      if (!PullChunk()) break;
    }
    auto n = min<size_t>(_buf_z - _buf_i, p_z - p_i);
    memcpy(p_i, _buf_i, n);
    _buf_i += n;
    p_i += n;
  }
  _offset += p_i - (u8*)p;
  return p_i - (u8*)p;
}

bool Lz4File::GetLine(void* p, int max_size) {
  --max_size;
  auto p_i = (u8*)p;
  auto p_z = (u8*)p + max_size;
  while (p_i < p_z) {
    if (_buf_i == _buf_z) {
      if (!PullChunk()) break;
    }
    while (_buf_i < _buf_z) {
      auto ch = *p_i++ = *_buf_i++;
      ++_offset;
      if (ch == '\n') {
        *p_i = 0;
        return true;
      }
    }
  }
  return false;
}

void Lz4File::Seek(i64 offset) {
  offset = clamp<i64>(offset, 0, _size);
  if (offset == _offset) return;
  if (offset < (i64)_offset) {
    if (_buf_i - _buf >= _offset - offset) {
      _buf_i -= _offset - offset;
      _offset = offset;
      return;
    }
    _archive_offset = _desc->_archive_offset + 4;
    _offset = 0;
    _buf_z = _buf_i = _buf;
  }
  u64 remain = offset - _offset;
  while (remain > 0) {
    if (_buf_i == _buf_z) {
      if (!PullChunk()) break;
    }
    auto n = min<u64>(_buf_z - _buf_i, remain);
    _buf_i += n;
    remain -= n;
    _offset += n;
  }
}

bool Lz4File::PullChunk() {
  platform_pread(_desc->_archive_fd, CPTR, 8, (off_t)_archive_offset);
  u32 c_size = ((u32*)CPTR)[0];
  u32 d_size = ((u32*)CPTR)[1];
  if (c_size == 0) return false;
  _archive_offset += 8;
  if (c_size & NC_MASK) {
    platform_pread(_desc->_archive_fd, _buf, d_size, (off_t)_archive_offset);
    _archive_offset += d_size;
  } else {
    platform_pread(_desc->_archive_fd, CPTR, c_size, (off_t)_archive_offset);
    _archive_offset += c_size;
    int ret = LZ4_decompress_fast((const char*)CPTR, (char*)_buf, d_size);
    c3_assert(ret == c_size);
  }
  _buf_i = _buf;
  _buf_z = _buf + d_size;
  return true;
}
