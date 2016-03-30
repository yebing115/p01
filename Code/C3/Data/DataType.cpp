#include "DataType.h"

bool IS_FIXED_POINT_TYPE[DATA_TYPE_COUNT] = {
  true, true, true, true, true, true, true, true, // i8, u8, ...
  false, false // float and double
};

u16 DATA_TYPE_SIZE[DATA_TYPE_COUNT] = {
  sizeof(i8), sizeof(u8),
  sizeof(i16), sizeof(u16),
  sizeof(i32), sizeof(u32),
  sizeof(i64), sizeof(u64),
  sizeof(float), sizeof(double)
};

const char* DATA_TYPE_NAMES[DATA_TYPE_COUNT] = {
  "INT8",
  "UINT8",
  "INT16",
  "UINT16",
  "INT32",
  "UINT32",
  "INT64",
  "UINT64",
  "FLOAT",
  "DOUBLE",
};
