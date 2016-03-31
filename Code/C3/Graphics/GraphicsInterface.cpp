#include "C3PCH.h"
#include "GraphicsInterface.h"
#include "GraphicsRenderer.h"
#include "Platform/PlatformConfig.h"
#if ON_WINDOWS
#include "D3D11/GraphicsInterfaceD3D11.h"
#elif ON_IOS
#include "graphics/gl/ios/GraphicsInterfaceGlIos.h"
#endif // ON_WINDOWS

#include "Debug/C3Debug.h"
#include "Data/DataType.h"

thread_local GraphicsInterface* GraphicsInterface::__instance = nullptr;

GraphicsInterface::GraphicsInterface(GraphicsAPI api, int major_version, int minor_version)
: _api(api), _major_version(major_version), _minor_version(minor_version) {

  for (int i = 0; i < C3_MAX_VIEWS; ++i) sprintf(_view_names[i], "%3d   ", i);
}

bool GraphicsInterface::CreateInstances(GraphicsAPI api, int major_version, int minor_version, bool need_auxiliary) {
  assert(!__instance);

  function<GraphicsInterface*()> main_instance_creator;
  switch (api) {
#if ON_WINDOWS
    case D3D11_WIN_API:
      main_instance_creator = [major_version, minor_version]() { return new GraphicsInterfaceD3D11(D3D11_WIN_API, major_version, minor_version); };
      break;
#elif ON_IOS
    case OPENGL_IOS_API:
      main_instance_creator = [major_version, minor_version]() { return new GraphicsInterfaceGlIos(major_version, minor_version); };
      auxiliary_instance_creator = [](const GraphicsInterface& main_instance) { return new GraphicsInterfaceGlIos(static_cast<const GraphicsInterfaceGlIos&>(main_instance)); };
      break;
#endif
#if 0
    case NULL_GRAPHICS_API:
      main_instance_creator = [major_version, minor_version]() { return new GraphicsInterfaceNull(NULL_API, major_version, minor_version); };
      break;
#endif
    default:
      c3_log("[C3] Unknown graphics api\n");
      return false;
      break;
  }
  assert(main_instance_creator);

  __instance = main_instance_creator();
  if (!__instance->OK()) {
    safe_delete(__instance);
    return false;
  }

  return true;
}

void GraphicsInterface::ReleaseInstances() {
  safe_delete(__instance);
}

void GraphicsInterface::UpdateViewName(u8 view, const char* name, int name_len) {
  c3_assert_return(name_len < C3_MAX_VIEW_NAME - C3_VIEW_NAME_RESERVED);
  memcpy(&_view_names[view][C3_VIEW_NAME_RESERVED], name, name_len + 1);
}

void GraphicsInterface::UpdateConstants(ConstantBuffer* constant_buffer, u32 begin, u32 end) {
  constant_buffer->Reset(begin);
  while (constant_buffer->GetPos() < end) {
    u32 opcode = constant_buffer->Read();

    if (opcode == CONSTANT_END) break;

    ConstantType type;
    u16 loc;
    u16 num;
    u16 copy;
    ConstantBuffer::DecodeOpcode(opcode, type, loc, num, copy);

    u32 size = CONSTANT_TYPE_SIZE[type] * num;
    const char* data = constant_buffer->Read(size);
    if (type < CONSTANT_COUNT) {
      if (copy) UpdateConstant(loc, data, size);
      else UpdateConstant(loc, *(const char**)(data), size);
    } else {
      SetMarker(data, size);
    }
  }
}

void ClearQuad::Init() {
  decl = VERTEX_DECLS[VERTEX_P3];
  auto GR = GraphicsRenderer::Instance();
#if 0
  Handle vsh;
  const MemoryBlock* fsh_mems[COLOR_ATTACHMENT_COUNT];
  if (AppConfig::GRAPHICS_API == DIRECTX_WIN_API) {
    vsh = GR->CreateShader(mem_ref(fs_clear_vsh_dx, sizeof(fs_clear_vsh_dx)));
    fsh_mems[0] = mem_ref(fs_clear0_fsh_dx, sizeof(fs_clear0_fsh_dx));
    fsh_mems[1] = mem_ref(fs_clear1_fsh_dx, sizeof(fs_clear1_fsh_dx));
    fsh_mems[2] = mem_ref(fs_clear2_fsh_dx, sizeof(fs_clear2_fsh_dx));
    fsh_mems[3] = mem_ref(fs_clear3_fsh_dx, sizeof(fs_clear3_fsh_dx));
    fsh_mems[4] = mem_ref(fs_clear4_fsh_dx, sizeof(fs_clear4_fsh_dx));
    fsh_mems[5] = mem_ref(fs_clear5_fsh_dx, sizeof(fs_clear5_fsh_dx));
    fsh_mems[6] = mem_ref(fs_clear6_fsh_dx, sizeof(fs_clear6_fsh_dx));
    fsh_mems[7] = mem_ref(fs_clear7_fsh_dx, sizeof(fs_clear7_fsh_dx));
  } else {
    vsh = GR->CreateShader(mem_ref(fs_clear_vsh_gl, sizeof(fs_clear_vsh_gl)));
    fsh_mems[0] = mem_ref(fs_clear0_fsh_gl, sizeof(fs_clear0_fsh_gl));
    fsh_mems[1] = mem_ref(fs_clear1_fsh_gl, sizeof(fs_clear1_fsh_gl));
    fsh_mems[2] = mem_ref(fs_clear2_fsh_gl, sizeof(fs_clear2_fsh_gl));
    fsh_mems[3] = mem_ref(fs_clear3_fsh_gl, sizeof(fs_clear3_fsh_gl));
    fsh_mems[4] = mem_ref(fs_clear4_fsh_gl, sizeof(fs_clear4_fsh_gl));
    fsh_mems[5] = mem_ref(fs_clear5_fsh_gl, sizeof(fs_clear5_fsh_gl));
    fsh_mems[6] = mem_ref(fs_clear6_fsh_gl, sizeof(fs_clear6_fsh_gl));
    fsh_mems[7] = mem_ref(fs_clear7_fsh_gl, sizeof(fs_clear7_fsh_gl));
  }
  for (u32 i = 0; i < COLOR_ATTACHMENT_COUNT; ++i) {
    auto fsh = GR->CreateShader(fsh_mems[i]);
    program[i] = GR->CreateProgram(vsh, fsh);
    if (!program[i]) c3_log("Failed to create clear quad program.\n");
    //GR->DestroyShader(fsh);
  }
  //GR->DestroyShader(vsh);
#endif
  vb = GR->CreateTransientVertexBuffer(4 * decl.stride, &decl);
}

void ClearQuad::Shutdown() {
  return;
#if 0
  auto GR = GraphicsRenderer::Instance();
  for (u32 i = 0; i < ATTACHMENT_POINT_COUNT; ++i) {
    if (program[i]) {
      GR->DestroyProgram(program[i]);
      program[i] = Handle();
    }
  }
  GR->DestroyTransientVertexBuffer(vb);
#endif
}
