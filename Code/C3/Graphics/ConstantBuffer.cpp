#include "C3PCH.h"
#include "ConstantBuffer.h"

u16 CONSTANT_TYPE_SIZE[CONSTANT_COUNT + 1] = {
  sizeof(i32),  // CONSTANT_BOOL
  sizeof(i32),  // CONSTANT_INT
  sizeof(float),  // CONSTANT_FLOAT
  sizeof(float) * 2,  // CONSTANT_VEC2
  sizeof(float) * 3,  // CONSTANT_VEC3
  sizeof(float) * 4,  // CONSTANT_VEC4
  sizeof(float) * 9,  // CONSTANT_MAT3
  sizeof(float) * 16,  // CONSTANT_MAT4
  0, // CONSTANT_END
  1, // CONSTANT_COUNT (used by marker)
};

void ConstantBuffer::WriteConstant(ConstantType type, u16 loc, const void* value, u16 num) {
  u32 opcode = EncodeOpcode(type, loc, num, true);
  Write(opcode);
  Write(value, CONSTANT_TYPE_SIZE[type] * num);
}

void ConstantBuffer::WriteConstantHandle(ConstantType type, u16 loc, Handle handle, u16 num) {
  u32 opcode = EncodeOpcode(type, loc, num, false);
  Write(opcode);
  Write(&handle, sizeof(Handle));
}

void ConstantBuffer::WriteMarker(const char* marker) {
  u16 num = (u16)strlen(marker)+1;
  u32 opcode = EncodeOpcode(CONSTANT_COUNT, 0, num, true);
  Write(opcode);
  Write(marker, num);
}
