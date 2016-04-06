#pragma once
#include "GraphicsTypes.h"
#include "Memory/C3Memory.h"

#define CONSTANT_OPCODE_TYPE_SHIFT 27
#define CONSTANT_OPCODE_TYPE_MASK  UINT32_C(0xf8000000)
#define CONSTANT_OPCODE_LOC_SHIFT  11
#define CONSTANT_OPCODE_LOC_MASK   UINT32_C(0x07fff800)
#define CONSTANT_OPCODE_NUM_SHIFT  1
#define CONSTANT_OPCODE_NUM_MASK   UINT32_C(0x000007fe)
#define CONSTANT_OPCODE_COPY_SHIFT 0
#define CONSTANT_OPCODE_COPY_MASK  UINT32_C(0x00000001)

#define CONSTANT_FRAGMENTBIT UINT8_C(0x10)
#define CONSTANT_SAMPLERBIT  UINT8_C(0x20)
#define CONSTANT_MASK (CONSTANT_FRAGMENTBIT | CONSTANT_SAMPLERBIT)

class ConstantBuffer {
public:
  static ConstantBuffer* Create(u32 size_ = 1 << 20) {
    u32 size = ALIGN_16(max<u32>(size_, sizeof(ConstantBuffer)));
    void* data = C3_ALLOC(g_allocator, size);
    return ::new(data)ConstantBuffer(size_);
  }

  static void Destroy(ConstantBuffer* constant_buffer) {
    constant_buffer->~ConstantBuffer();
    C3_FREE(g_allocator, constant_buffer);
  }

  static void Update(ConstantBuffer*& constant_buffer, u32 threshold = 64 << 10, u32 grow = 1 << 20) {
    if (threshold >= constant_buffer->_size - constant_buffer->_pos) {
      u32 size = ALIGN_16(max<u32>(constant_buffer->_size + grow, sizeof(ConstantBuffer)));
      void* data = C3_REALLOC(g_allocator, constant_buffer, size);
      constant_buffer = (ConstantBuffer*)data;
      constant_buffer->_size = size;
    }
  }

  static u32 EncodeOpcode(ConstantType type_, u16 loc_, u16 num_, u16 copy_) {
    const u32 type = type_ << CONSTANT_OPCODE_TYPE_SHIFT;
    const u32 loc = loc_ << CONSTANT_OPCODE_LOC_SHIFT;
    const u32 num = num_ << CONSTANT_OPCODE_NUM_SHIFT;
    const u32 copy = copy_ << CONSTANT_OPCODE_COPY_SHIFT;
    return type | loc | num | copy;
  }

  static void DecodeOpcode(u32 opcode_, ConstantType& type_out, u16& loc_out, u16& num_out, u16& copy_out) {
    const u32 type = (opcode_ & CONSTANT_OPCODE_TYPE_MASK) >> CONSTANT_OPCODE_TYPE_SHIFT;
    const u32 loc = (opcode_ & CONSTANT_OPCODE_LOC_MASK) >> CONSTANT_OPCODE_LOC_SHIFT;
    const u32 num = (opcode_ & CONSTANT_OPCODE_NUM_MASK) >> CONSTANT_OPCODE_NUM_SHIFT;
    const u32 copy = (opcode_ & CONSTANT_OPCODE_COPY_MASK); // >> CONSTANT_OPCODE_COPY_SHIFT;

    type_out = (ConstantType)(type);
    copy_out = (u16)copy;
    num_out = (u16)num;
    loc_out = (u16)loc;
  }

  void Write(const void* data, u32 size) {
    if (_pos + size < _size) {
      ::memcpy(&_buffer[_pos], data, size);
      _pos += size;
    }
  }

  void Write(u32 _value) { Write(&_value, sizeof(u32)); }

  const char* Read(u32 size) {
    const char* result = &_buffer[_pos];
    _pos += size;
    return result;
  }

  u32 Read() {
    u32 result;
    ::memcpy(&result, Read(sizeof(u32)), sizeof(u32));
    return result;
  }

  bool IsEmpty() const { return _pos == 0; }
  u32 GetPos() const { return _pos; }
  void Reset(u32 pos = 0) { _pos = pos; }
  void Finish() {
    Write(CONSTANT_END);
    _pos = 0;
  }

  void WriteConstant(ConstantType type, u16 loc, const void* value, u16 num = 1);
  void WriteConstantHandle(ConstantType type, u16 loc, ConstantHandle handle, u16 num = 1);
  void WriteMarker(const char* marker);

private:
  ConstantBuffer(u32 size): _size(size - sizeof(_buffer)), _pos(0) { Finish(); }
  ~ConstantBuffer() {}
  u32 _size;
  u32 _pos;
  char _buffer[8];
};

struct ConstantInfo {
  const void* data;
  ConstantHandle handle;
};

class ConstantRegistry {
public:
  ConstantRegistry() {}
  ~ConstantRegistry() {}

  const ConstantInfo* Find(stringid name) const {
    ConstantHashMap::const_iterator it = _constants.find(name);
    if (it != _constants.end()) return &it->second;
    return nullptr;
  }

  const ConstantInfo& Add(ConstantHandle handle, stringid name, const void* data) {
    ConstantHashMap::iterator it = _constants.find(name);
    if (it == _constants.end()) {
      ConstantInfo info;
      info.data = data;
      info.handle = handle;

      auto result = _constants.insert(ConstantHashMap::value_type(name, info));
      return result.first->second;
    }

    ConstantInfo& info = it->second;
    info.data = data;
    info.handle = handle;

    return info;
  }

private:
  typedef unordered_map<stringid, ConstantInfo> ConstantHashMap;
  ConstantHashMap _constants;
};

extern u16 CONSTANT_TYPE_SIZE[CONSTANT_COUNT + 1];
