#pragma once
#include "Data/DataType.h"

#define RADIX_SORT_BITS 11
#define RADIX_SORT_HISTOGRAM_SIZE (1<<RADIX_SORT_BITS)
#define RADIX_SORT_BIT_MASK (RADIX_SORT_HISTOGRAM_SIZE-1)

template <typename T>
void radix_sort32(u32* __restrict keys, u32* __restrict temp_keys, T* __restrict values, T* __restrict temp_values, u32 size) {
  u32 histogram[RADIX_SORT_HISTOGRAM_SIZE];
  u16 shift = 0;
  u32 pass = 0;
  for (; pass < 3; ++pass) {
    memset(histogram, 0, sizeof(u32) * RADIX_SORT_HISTOGRAM_SIZE);

    bool sorted = true;
    {
      u32 key = keys[0];
      u32 prevKey = key;
      for (u32 ii = 0; ii < size; ++ii, prevKey = key) {
        key = keys[ii];
        u16 index = (key >> shift) & RADIX_SORT_BIT_MASK;
        ++histogram[index];
        sorted &= prevKey <= key;
      }
    }

    if (sorted) goto done;

    u32 offset = 0;
    for (u32 ii = 0; ii < RADIX_SORT_HISTOGRAM_SIZE; ++ii) {
      u32 count = histogram[ii];
      histogram[ii] = offset;
      offset += count;
    }

    for (u32 ii = 0; ii < size; ++ii) {
      u32 key = keys[ii];
      u16 index = (key >> shift) & RADIX_SORT_BIT_MASK;
      u32 dest = histogram[index]++;
      temp_keys[dest] = key;
      temp_values[dest] = values[ii];
    }

    u32* swap_keys = temp_keys;
    temp_keys = keys;
    keys = swap_keys;

    T* swap_values = temp_values;
    temp_values = values;
    values = swap_values;

    shift += RADIX_SORT_BITS;
  }

done:
  if (0 != (pass & 1)) {
    // Odd number of passes needs to do copy to the destination.
    memcpy(keys, temp_keys, size * sizeof(u32));
    for (u32 ii = 0; ii < size; ++ii) {
      values[ii] = temp_values[ii];
    }
  }
}

template <typename T>
void radix_sort64(u64* __restrict keys, u64* __restrict temp_keys, T* __restrict values, T* __restrict temp_values, u32 size) {
  u32 histogram[RADIX_SORT_HISTOGRAM_SIZE];
  u16 shift = 0;
  u32 pass = 0;
  for (; pass < 6; ++pass) {
    memset(histogram, 0, sizeof(u32)*RADIX_SORT_HISTOGRAM_SIZE);

    bool sorted = true;
    {
      u64 key = keys[0];
      u64 prevKey = key;
      for (u32 ii = 0; ii < size; ++ii, prevKey = key) {
        key = keys[ii];
        u16 index = (key >> shift) & RADIX_SORT_BIT_MASK;
        ++histogram[index];
        sorted &= prevKey <= key;
      }
    }

    if (sorted) goto done;

    u32 offset = 0;
    for (u32 ii = 0; ii < RADIX_SORT_HISTOGRAM_SIZE; ++ii) {
      u32 count = histogram[ii];
      histogram[ii] = offset;
      offset += count;
    }

    for (u32 ii = 0; ii < size; ++ii) {
      u64 key = keys[ii];
      u16 index = (key >> shift) & RADIX_SORT_BIT_MASK;
      u32 dest = histogram[index]++;
      temp_keys[dest] = key;
      temp_values[dest] = values[ii];
    }

    u64* swap_keys = temp_keys;
    temp_keys = keys;
    keys = swap_keys;

    T* swap_values = temp_values;
    temp_values = values;
    values = swap_values;

    shift += RADIX_SORT_BITS;
  }

done:
  if (0 != (pass & 1)) {
    // Odd number of passes needs to do copy to the destination.
    memcpy(keys, temp_keys, size * sizeof(u64));
    for (u32 ii = 0; ii < size; ++ii) {
      values[ii] = temp_values[ii];
    }
  }
}

#undef RADIX_SORT_BITS
#undef RADIX_SORT_HISTOGRAM_SIZE
#undef RADIX_SORT_BIT_MASK


