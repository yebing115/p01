#pragma once
#include "Platform/PlatformConfig.h"
struct CommandBuffer {
  enum CommandType {
    RENDERER_INIT,
    RENDERER_SHUTDOWN_BEGIN,
    CREATE_VERTEX_DECL,
    CREATE_INDEX_BUFFER,
    CREATE_VERTEX_BUFFER,
    CREATE_DYNAMIC_VERTEX_BUFFER,
    CREATE_DYNAMIC_INDEX_BUFFER,
    UPDATE_DYNAMIC_VERTEX_BUFFER,
    UPDATE_DYNAMIC_INDEX_BUFFER,
    CREATE_SHADER,
    CREATE_PROGRAM,
    CREATE_TEXTURE,
    UPDATE_TEXTURE,
    RESIZE_TEXTURE,
    CREATE_FRAME_BUFFER,
    CREATE_CONSTANT,
    UPDATE_VIEW_NAME,
    
    END,
    
    RENDERER_SHUTDOWN_END,
    DESTROY_VERTEX_DECL,
    DESTROY_INDEX_BUFFER,
    DESTROY_DYNAMIC_INDEX_BUFFER,
    DESTROY_VERTEX_BUFFER,
    DESTROY_DYNAMIC_VERTEX_BUFFER,
    DESTROY_SHADER,
    DESTROY_PROGRAM,
    DESTROY_TEXTURE,
    DESTROY_FRAME_BUFFER,
    DESTROY_CONSTANT,
    SAVE_SCREENSHOT,
  };

  CommandBuffer(): pos(0), size(C3_MAX_COMMAND_BUFFER_SIZE) {
    Finish();
  }

  void Write(const void* data, u32 size_) {
    c3_assert(size == C3_MAX_COMMAND_BUFFER_SIZE && "Called write outside start/finish?");
    c3_assert(pos < size);
    memcpy(buffer + pos, data, size_);
    pos += size_;
  }

  template<typename T>
  void Write(const T& val) {
    Align(ALIGN_OF(T));
    Write(&val, sizeof(T));
  }

  void Read(void* data, u32 size_) {
    c3_assert(pos < size);
    memcpy(data, buffer + pos, size_);
    pos += size_;
  }

  template<typename T>
  void Read(T& val) {
    Align(ALIGN_OF(T));
    Read((void*)&val, sizeof(T));
  }

  const void* Skip(u32 size_) {
    c3_assert(pos < size);
    const void* result = buffer + pos;
    pos += size_;
    return result;
  }

  template<typename T>
  void Skip() {
    Align(ALIGN_OF(T));
    Skip(sizeof(T));
  }

  void Align(u32 alignment) {
    const u32 mask = alignment - 1;
    const u32 new_pos = (pos + mask) & (~mask);
    pos = new_pos;
  }

  void Reset() {
    pos = 0;
  }

  void Start() {
    pos = 0;
    size = C3_MAX_COMMAND_BUFFER_SIZE;
  }

  void Finish() {
    u8 cmd = END;
    Write(cmd);
    size = pos;
    pos = 0;
  }

  u32 pos;
  u32 size;
  u8 buffer[C3_MAX_COMMAND_BUFFER_SIZE];

private:
  CommandBuffer(const CommandBuffer&) = delete;
  void operator=(const CommandBuffer&) = delete;
};
