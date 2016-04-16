#pragma once
#include "Data/DataType.h"

enum HandleType {
  INVALID_HANDLE = -1,
  
  TEXTURE_HANDLE,
  FRAME_BUFFER_HANDLE,
  VERTEX_DECL_HANDLE,
  VERTEX_BUFFER_HANDLE,
  DYNAMIC_VERTEX_BUFFER_HANDLE,
  INDEX_BUFFER_HANDLE,
  DYNAMIC_INDEX_BUFFER_HANDLE,
  SHADER_HANDLE,
  PROGRAM_HANDLE,
  CONSTANT_HANDLE,
  
  COMPONENT_HANDLE_START,
  ENTITY_HANDLE = COMPONENT_HANDLE_START,
  TRANSFORM_HANDLE,
  CAMERA_HANDLE,
  MODEL_RENDERER_HANDLE,
  LIGHT_HANDLE,
  COMPONENT_HANDLE_END,

  NUM_HANDLE_TYPES = COMPONENT_HANDLE_END
};
//#define NUM_COMPONENT_HANDLE_TYPES (COMPONENT_HANDLE_END - COMPONENT_HANDLE_START)

#define INVALID_HANDLE_RAW UINT32_MAX

#define HANDLE_INDEX_BIT_WIDTH  16
#define HANDLE_AGE_BIT_WIDTH    8
#define HANDLE_TYPE_BIT_WIDTH   8
#define RAW_HANDLE_TYPE         u32

struct GenericHandle {
  RAW_HANDLE_TYPE idx : HANDLE_INDEX_BIT_WIDTH;
  RAW_HANDLE_TYPE age : HANDLE_AGE_BIT_WIDTH;
  RAW_HANDLE_TYPE type : HANDLE_TYPE_BIT_WIDTH;

  GenericHandle() { Reset(); }
  explicit GenericHandle(RAW_HANDLE_TYPE raw) { *((RAW_HANDLE_TYPE*)this) = raw; }
  RAW_HANDLE_TYPE ToRaw() const { return *((RAW_HANDLE_TYPE*)this); }
  void SetRaw(RAW_HANDLE_TYPE raw) { *((RAW_HANDLE_TYPE*)this) = raw; }
  operator bool() const { return IsValid(); }
  explicit operator RAW_HANDLE_TYPE() const { return ToRaw(); }
  bool IsValid() const { return *((u32*)this) != INVALID_HANDLE_RAW; }
  bool IsEntity() const { return type == ENTITY_HANDLE; }
  bool Is(HandleType t) const { return type == t; }
  void Reset() { *((RAW_HANDLE_TYPE*)this) = INVALID_HANDLE_RAW; }
};

template <HandleType TYPE>
struct Handle : public GenericHandle {
  Handle() {}
  explicit Handle(RAW_HANDLE_TYPE raw) : GenericHandle(raw) {}
};

namespace std {
template <HandleType TYPE>
struct hash<Handle<TYPE>> {
  size_t operator ()(const Handle<TYPE>& h) const { return h.ToRaw(); }
};
}

typedef Handle<TEXTURE_HANDLE> TextureHandle;
typedef Handle<FRAME_BUFFER_HANDLE> FrameBufferHandle;
typedef Handle<VERTEX_DECL_HANDLE> VertexDeclHandle;
typedef Handle<VERTEX_BUFFER_HANDLE> VertexBufferHandle;
typedef Handle<DYNAMIC_VERTEX_BUFFER_HANDLE> DynamicVertexBufferHandle;
typedef Handle<INDEX_BUFFER_HANDLE> IndexBufferHandle;
typedef Handle<DYNAMIC_INDEX_BUFFER_HANDLE> DynamicIndexBufferHandle;
typedef Handle<SHADER_HANDLE> ShaderHandle;
typedef Handle<PROGRAM_HANDLE> ProgramHandle;
typedef Handle<CONSTANT_HANDLE> ConstantHandle;

typedef Handle<ENTITY_HANDLE> EntityHandle;
typedef unordered_map<EntityHandle, int> EntityMap;
