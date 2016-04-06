#pragma once
#include "Pattern/Handle.h"

#define MAX_RENDER_ITEM_BINDING_COUNT 9

struct Binding {
  enum Type {
    Image,
    IndexBuffer,
    VertexBuffer,
    Texture,

    Count
  };
  u16 idx;
  u8 type;
  u32 flags;
};

struct RenderItem {
  VertexBufferHandle vertex_buffer;
  //Handle instance_data_buffer;
  IndexBufferHandle index_buffer;
  VertexDeclHandle vertex_decl;
  u32 start_vertex;
  u32 num_vertices;
  u32 start_index;
  u32 num_indices;
  u32 constant_begin;
  u32 constant_end;
  u64 flags;
  u16 program;
  u16 matrix;
  u16 num;
  u16 scissor;
  u32 rgba;
  u32 instance_data_offset;
  u16 instance_data_stride;
  u16 num_instances;
  Binding bind[MAX_RENDER_ITEM_BINDING_COUNT];
  void Clear();
};
