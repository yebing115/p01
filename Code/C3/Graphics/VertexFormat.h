#pragma once
#include "Data/DataType.h"
#include "Data/String.h"
#include "GraphicsTypes.h"
#include "Algorithm/Hasher.h"
#include "Debug/C3Debug.h"

struct VertexAttribute {
  int index;
  String name;
  DataType data_type;
  int data_count;

  int TotalSize() const { return DATA_TYPE_SIZE[data_type] * data_count; }
  
  static const int PADDING_INDEX = -1;
};

typedef vector<VertexAttribute> VertexAttributeList;

extern int calculate_vertex_size(const VertexAttributeList& attribute_list, int start, int end);
extern int calculate_vertex_size(const VertexAttributeList& attribute_list);

enum VertexFormat {
  INVALID_VERTEX_FORMAT = -1,
  VERTEX_P2,
  VERTEX_P2_T2,
  VERTEX_P3,
  VERTEX_P3_T2,
  VERTEX_P3_T2_C4,
  VERTEX_P3_N3,
  VERTEX_P3_N3_T2,
  VERTEX_P2_T2_I1,
  VERTEX_P3_T2_J4,
  VERTEX_FORMAT_COUNT
};

#pragma pack(push, 1)
struct VertexP2 {
  float2 position;
};

struct VertexP3 {
  float3 position;
};

struct VertexP3T2 {
  float3 position;
  float2 texcoord;
};

struct VertexP3T2C4 {
  float3 position;
  float2 texcoord;
  float4 color;
};

struct VertexP3N3 {
  float3 position;
  float3 normal;
};

struct VertexP3N3T2 {
  float3 position;
  float3 normal;
  float2 texcoord;
};

struct VertexP2T2 {
  float2 position;
  float2 texcoord;
};

struct VertexP2T2I1 {
  float2 position;
  float2 texcoord;
  i8 index;
  u8 padding[3];
};

struct VertexP3T2J4 {
  float3 position;
  float2 texcoord;
  i8 joint_id[4];
  float weight[4];
};
#pragma pack(pop)

enum VertexAttr {
  VERTEX_ATTR_POSITION,
  VERTEX_ATTR_NORMAL,
  VERTEX_ATTR_TANGENT,
  VERTEX_ATTR_BITANGENT,
  VERTEX_ATTR_COLOR0,
  VERTEX_ATTR_COLOR1,
  VERTEX_ATTR_TEXCOORD0,
  VERTEX_ATTR_TEXCOORD1,
  VERTEX_ATTR_INDEX,
  VERTEX_ATTR_WEIGHT,
  VERTEX_ATTR_COUNT,
};
extern const char* VERTEX_ATTR_NAMES[VERTEX_ATTR_COUNT];

enum InstanceAttr {
  INSTANCE_ATTR_I_DATA0 = VERTEX_ATTR_COUNT,
  INSTANCE_ATTR_I_DATA1,
  INSTANCE_ATTR_I_DATA2,
  INSTANCE_ATTR_I_DATA3,
  INSTANCE_ATTR_I_DATA4,
  INSTANCE_ATTR_COUNT = 5,
};

struct VertexDecl {
  VertexDecl& Begin(GraphicsAPI api = NULL_GRAPHICS_API) {
    hash = api;
    stride = 0;
    memset(offsets, 0, sizeof(offsets));
    memset(attributes, 0xff, sizeof(attributes));
    return *this;
  }
  void End() {
    Hasher h;
    h.Begin();
    h.Add(offsets);
    h.Add(attributes);
    hash = h.End();
  }
  VertexDecl& Add(VertexAttr attrib, u8 num, DataType data_type, bool normalized = false, bool as_int = false) {
    const u16 encoded_norm = (normalized & 1) << 7;
    const u16 encoded_type = (data_type & 15) << 3;
    const u16 encoded_num = (num - 1) & 3;
    const u16 encode_as_int = ((as_int && IS_FIXED_POINT_TYPE[data_type]) & 1) << 8;
    attributes[attrib] = encoded_norm | encoded_type | encoded_num | encode_as_int;

    offsets[attrib] = stride;
    // #todo: data_type size depends on GraphicsInterface
    // #todo: padding depends on 'data_type' and 'num'
    stride += DATA_TYPE_SIZE[data_type] * num;

    return *this;
  }
  VertexDecl& Skip(u8 size) {
    stride += size;
    return *this;
  }
  bool Query(VertexAttr attrib, u8& num_out, DataType& data_type_out, bool& normalized_out, bool& as_int_out) const {
    if (attributes[attrib] == UINT16_MAX) return false;
    u16 val = attributes[attrib];
    num_out = (val & 3) + 1;
    data_type_out = (DataType)((val >> 3) & 15);
    normalized_out = !!(val & (1 << 7));
    as_int_out = !!(val & (1 << 8));
    return true;
  }
  void Dump() const {
    c3_log("VertexDecl stride = %d hash = %08x\n", stride, hash);
    for (int i = 0; i < VERTEX_ATTR_COUNT; ++i) {
      u8 num;
      DataType data_type;
      bool normalized;
      bool as_int;
      if (Query((VertexAttr)i, num, data_type, normalized, as_int)) {
        c3_log("%12s, offset = %2d, %6s%d %c%c\n",
               VERTEX_ATTR_NAMES[i],
               offsets[i],
               DATA_TYPE_NAMES[data_type],
               (int)num,
               normalized ? 'N' : ' ',
               as_int ? 'I' : ' ');
      }
    }
  }

  u32 hash;
  u16 stride;
  u16 offsets[VERTEX_ATTR_COUNT];
  u16 attributes[VERTEX_ATTR_COUNT];
};

extern VertexDecl VERTEX_DECLS[VERTEX_FORMAT_COUNT];
void init_builtin_vertex_decls(GraphicsAPI api);

int semantic_to_vertex_attr(stringid semantic);
stringid vertex_attr_to_id(int attr);
