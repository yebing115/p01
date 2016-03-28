#pragma once
#include "Data/DataType.h"
#include "Platform/C3Platform.h"

// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#define MURMUR_M 0x5bd1e995
#define MURMUR_R 24
#define mmix(h, k) { k *= MURMUR_M; k ^= k >> MURMUR_R; k *= MURMUR_M; h *= MURMUR_M; h ^= k; }

class Hasher {
public:
  void Begin(u32 seed = 0) {
    _hash = seed;
    _tail = 0;
    _count = 0;
    _size = 0;
  }

  void Add(const void* data, int len) {
    AddAligned(data, len);
  }

  void AddAligned(const void* data_, int len) {
    const u8* data = (const u8*)data_;
    _size += len;

    MixTail(data, len);

    while (len >= 4) {
      u32 kk = *(u32*)data;

      mmix(_hash, kk);

      data += 4;
      len -= 4;
    }

    MixTail(data, len);
  }

  void AddUnaligned(const void* data_, int len) {
    const u8* data = (u8*)data_;
    _size += len;

    MixTail(data, len);

    while (len >= 4) {
      u32 kk;
      ReadUnaligned(data, kk);

      mmix(_hash, kk);

      data += 4;
      len -= 4;
    }

    MixTail(data, len);
  }

  template<typename T>
  void Add(T value) {
    Add((const void*)&value, sizeof(T));
  }

  u32 End() {
    mmix(_hash, _tail);
    mmix(_hash, _size);

    _hash ^= _hash >> 13;
    _hash *= MURMUR_M;
    _hash ^= _hash >> 15;

    return _hash;
  }

private:
  static void ReadUnaligned(const void* data_, u32& out) {
    const u8* data = (const u8*)data_;
#if CPU_ENDIAN_LITTLE
      out = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
#else
      out = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
#endif
  }

  void MixTail(const u8*& data, int& len) {
    while (len && ((len < 4) || _count)) {
      _tail |= (*data++) << (_count * 8);

      _count++;
      len--;

      if (_count == 4) {
        mmix(_hash, _tail);
        _tail = 0;
        _count = 0;
      }
    }
  }

  u32 _hash;
  u32 _tail;
  u32 _count;
  u32 _size;
};

#undef MURMUR_M
#undef MURMUR_R
#undef mmix

inline u32 hash_buffer(const void* data, u32 size) {
  Hasher murmur;
  murmur.Begin();
  murmur.Add(data, (int)size);
  return murmur.End();
}

inline u32 hash_string(const char* s) {
  u32 h = 0x811c9dc5;
  while (u32 c = *s++) {
    h ^= c;
    h *= 0x1000193;
  }
  return h;
}
