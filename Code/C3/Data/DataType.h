#pragma once

#include <stdint.h>
#include <vector>
#include <array>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <algorithm>
#include <functional>
#include <MathGeoLib.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;

using std::vector;
using std::array;
using std::list;
using std::unordered_map;
using std::unordered_set;
using std::min;
using std::max;
using std::move;
using std::sort;
using std::swap;
using std::make_pair;
using std::make_shared;
using std::function;
using std::bitset;

#define ALIGN_OF(type) __alignof(type)
#define STRUCT_OFFSET(StructName, field_name) ((int) (&(((StructName*) 0)->field_name)))
#define MAKE_FOURCC(a, b, c, d) (((u32)(a) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24)))
template <typename T, size_t N>  constexpr size_t ARRAY_SIZE(const T(&)[N]) { return N; }

enum DataType {
  INVALID_DATA_TYPE = -1,
  INT8_TYPE,
  UINT8_TYPE,
  INT16_TYPE,
  UINT16_TYPE,
  INT32_TYPE,
  UINT32_TYPE,
  INT64_TYPE,
  UINT64_TYPE,
  FLOAT_TYPE,
  DOUBLE_TYPE,
  DATA_TYPE_COUNT
};

extern bool IS_FIXED_POINT_TYPE[DATA_TYPE_COUNT];
extern u16 DATA_TYPE_SIZE[DATA_TYPE_COUNT];
extern const char* DATA_TYPE_NAMES[DATA_TYPE_COUNT];
