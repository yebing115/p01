#pragma once

#include "Graphics/Material/Material.h"

#define MAX_MESH_ATTRS  10

enum MeshAttrFlag {
  MESH_ATTR_DEFAULT = 0,
  MESH_ATTR_NORMALIZED = 1,
  MESH_ATTR_AS_INT = 2,
};

#pragma pack(push, 1)
/************************************************************************/
/* MeshHeader
/* MeshStringTable + strings
/* MeshMaterial[num_materials]
/* MeshPart[num_parts]
/* VertexData
/* IndexData
/************************************************************************/
struct MeshStringTable {
  u32 size;
  char data[];
};

struct MeshAttr {
  u8 attr;
  u8 num;
  u8 data_type;
  u8 flags;
};

struct MeshMaterialParam {
  MaterialParamType type;
  union {
    vec _vec;
    struct {
      stringid _filename;
      u32 _flags;
    } _tex2d;
  };
};

struct MeshMaterial {
  stringid material_shader;
  u32 num_params;
  MeshMaterialParam params;
};

struct MeshPart {
  stringid name;
  stringid material;
  u32 start_index;
  u32 num_indices;
  AABB aabb;
};

struct MeshHeader {
  u32 magic;
  u32 num_vertices;
  u32 num_indices;
  AABB aabb;
  u16 vertex_stride;
  u16 num_attrs;
  u16 num_materials;
  u16 num_parts;
  u32 material_data_offset;
  u32 part_data_offset;
  u32 vertex_data_offset;
  u32 index_data_offset;
  MeshAttr attrs[MAX_MESH_ATTRS];
};
static_assert(sizeof(MeshHeader) == 100, "Bad sizeof MeshHeader.");
#pragma pack(pop)
