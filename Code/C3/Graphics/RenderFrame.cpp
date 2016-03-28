#include "RenderFrame.h"
#include "ConstantBuffer.h"
#include "RenderKey.h"
#include "Graphics/GraphicsRenderer.h"
#include "Algorithm/C3Algorithm.h"

static u64 s_temp_keys[C3_MAX_DRAW_CALLS];
static u16 s_temp_values[C3_MAX_DRAW_CALLS];

RenderFrame::RenderFrame(): constant_buffer(nullptr), constant_max(0), render_item_count(0) {}

RenderFrame::~RenderFrame() {}

void RenderFrame::Create() {
  constant_buffer = ConstantBuffer::Create();
  memset(scissor, 0, sizeof(scissor));
  memset(rect, 0, sizeof(rect));
  memset(&current, 0, sizeof(current));
  Reset();
  Start();
}

void RenderFrame::Destroy() {
  ConstantBuffer::Destroy(constant_buffer);
  constant_buffer = nullptr;
}

void RenderFrame::Start() {
  constant_buffer->Reset();
  constant_begin = 0;
  constant_end = 0;
  current.Clear();
  render_item_count = 0;
  matrix_cache.Reset();
  rect_cache.Reset();
  pre_cmd_buffer.Start();
  post_cmd_buffer.Start();
  vb_offset = 0;
  ib_offset = 0;
  discard = false;
}

void RenderFrame::Finish() {
  pre_cmd_buffer.Finish();
  post_cmd_buffer.Finish();
  constant_max = max(constant_max, constant_buffer->GetPos());
  constant_buffer->Finish();
}

void RenderFrame::Clear() {
  render_item_count = 0;
}

void RenderFrame::Reset() {
  ResetFreeHandles();
  Start();
  Finish();
}

void RenderFrame::Sort() {
  for (u16 i = 0, n = render_item_count; i < n; ++i) {
    sort_keys[i] = SortKey::RemapView(sort_keys[i], view_remap);
  }
  radix_sort64(sort_keys, s_temp_keys, sort_values, s_temp_values, render_item_count);
}

void RenderFrame::Discard() {
  discard = true;
  current.Clear();
}

void RenderFrame::SetMarker(const char* marker) { constant_buffer->WriteMarker(marker); }

void RenderFrame::SetVertexBuffer(Handle handle, u32 vertex_start, u32 vertex_size) {
  current.start_vertex = vertex_start;
  current.num_vertices = vertex_size;
  current.vertex_buffer = handle;
}

void RenderFrame::SetVertexBuffer(const TransientVertexBuffer* tvb, u32 start_vertex, u32 num_vertices) {
  current.start_vertex = tvb->start_vertex + start_vertex;
  current.num_vertices = min(tvb->size / tvb->stride, num_vertices);
  current.vertex_buffer = tvb->handle;
  current.vertex_decl = tvb->decl;
}

void RenderFrame::SetState(u64 state, u32 rgba) {
  // transparency sort order table
  u8 blend = ((state & C3_STATE_BLEND_MASK) >> C3_STATE_BLEND_SHIFT) & 0xff;
  //sort_key.trans = "\x0\x1\x1\x2\x2\x1\x2\x1\x2\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1"[(blend & 0xf) + (!!blend)];
  sort_key.trans = (!!blend);
  current.flags = state;
  current.rgba = rgba;
}

void RenderFrame::SetIndexBuffer(Handle handle, u32 start_index, u32 num_indices) {
  current.start_index = start_index;
  current.num_indices = num_indices;
  current.index_buffer = handle;
}

void RenderFrame::SetIndexBuffer(const TransientIndexBuffer* tib, u32 first_index, u32 num_indices) {
  current.start_index = tib->start_index + first_index;
  current.num_indices = min(tib->size / 2, num_indices);
  current.index_buffer = tib->handle;
  discard = num_indices == 9;
}

void RenderFrame::SetTexture(u8 unit, Handle handle, u32 flags) {
  current.bind[unit].idx = (u16)handle.idx;
  current.bind[unit].type = Binding::Texture;
  current.bind[unit].flags = (flags & C3_SAMPLER_DEFAULT_FLAGS) ? C3_SAMPLER_DEFAULT_FLAGS : flags;
}

void RenderFrame::SetConstant(ConstantType type, Handle handle, const void* value, u16 num) {
  ConstantBuffer::Update(constant_buffer);
  constant_buffer->WriteConstant(type, (u16)handle.idx, value, num);
}

void RenderFrame::SetScissor(i16 x, i16 y, i16 width, i16 height) {
  current.scissor = (u16)rect_cache.Add(x, y, width, height);
}

u16 RenderFrame::SetTransform(const void* m, u16 num) {
  current.matrix = matrix_cache.Add(m, num);
  current.num = num;
  return current.matrix;
}

void RenderFrame::SetTransform(u16 cache, u16 num) {
  current.matrix = cache;
  current.num = num;
}

u16 RenderFrame::AllocTransform(float4x4*& m_out, u16& num_in_out) {
  u16 first = matrix_cache.Reserve(&num_in_out);
  m_out = (float4x4*)matrix_cache.GetBuffer(first);
  return first;
}

bool RenderFrame::CheckAvailTransientIndexBuffer(u32 num) {
  u32 offset = ib_offset;
  u32 iboffset = offset + num * sizeof(u16);
  iboffset = min<u32>(iboffset, C3_TRANSIENT_INDEX_BUFFER_SIZE);
  u32 n = (iboffset - offset) / sizeof(u16);
  return n == num;
}

u32 RenderFrame::AllocTransientIndexBuffer(u32& num_in_out) {
  u32 offset = num_align(ib_offset, sizeof(u16));
  ib_offset = offset + num_in_out * sizeof(u16);
  ib_offset = min<u32>(ib_offset, C3_TRANSIENT_INDEX_BUFFER_SIZE);
  num_in_out = (ib_offset - offset) / sizeof(u16);
  return offset;
}

bool RenderFrame::CheckAvailTransientVertexBuffer(u32 num, u16 stride) {
  u32 offset = num_align(vb_offset, stride);
  u32 vboffset = offset + num * stride;
  u32 n = (vboffset - offset) / stride;
  return n == num;
}

u32 RenderFrame::AllocTransientVertexBuffer(u32& num_in_out, u16 stride) {
  u32 offset = num_align(vb_offset, stride);
  vb_offset = offset + num_in_out * stride;
  vb_offset = min<u32>(vb_offset, C3_TRANSIENT_VERTEX_BUFFER_SIZE);
  num_in_out = (vb_offset - offset) / stride;
  return offset;
}

void RenderFrame::SetInstanceDataBuffer(const InstanceDataBuffer* idb, u32 num) {
  current.instance_data_offset = idb->offset;
  current.instance_data_stride = idb->stride;
  current.num_instances = (u16)min(idb->num, num);
  current.instance_data_buffer = idb->handle;
  C3_FREE(g_allocator, const_cast<InstanceDataBuffer*>(idb));
}

void RenderFrame::SetInstanceDataBuffer(Handle handle, u32 start_vertex, u32 num, u16 stride) {
  current.instance_data_offset = start_vertex * stride;
  current.instance_data_stride = stride;
  current.num_vertices = num;
  current.instance_data_buffer = handle;
}

void RenderFrame::SetViewName(u8 view, const char* name) {
  CommandBuffer& cmd = GetCommandBuffer(CommandBuffer::UPDATE_VIEW_NAME);
  cmd.Write(view);
  u8 len = (u8)strlen(name);
  cmd.Write(len);
  cmd.Write((const u8*)name, len + 1);
}

void RenderFrame::Submit(u8 view, Handle program, i32 depth) {
  if (discard) {
    Discard();
    return;
  }
  constant_end = constant_buffer->GetPos();
  current.constant_begin = constant_begin;
  current.constant_end = constant_end;
  render_items[render_item_count] = current;
  
  sort_key.depth = (u32)depth;
  sort_key.view = view;
  sort_key.program = (u16)program.idx;
  sort_key.seq = GraphicsRenderer::Instance()->_seq[view]++;
  if (!GraphicsRenderer::Instance()->_seq_enabled[view]) sort_key.seq = 0;
  u64 key = sort_key.EncodeDraw();
  sort_keys[render_item_count] = key;
  sort_values[render_item_count] = render_item_count;
  ++render_item_count;
  
  current.Clear();
  constant_begin = constant_end;
}

void RenderFrame::ResetFreeHandles() {
  num_free_index_buffer_handles = 0;
  num_free_vertex_decl_handles = 0;
  num_free_vertex_buffer_handles = 0;
  num_free_shader_handles = 0;
  num_free_program_handles = 0;
  num_free_texture_handles = 0;
  num_free_constant_handles = 0;
  num_free_frame_buffer_handles = 0;
}

stringid PredefinedConstant::TypeToName(PredefinedConstantType type) {
  if (type < PREDEFINED_CONSTANT_COUNT) return 0;
  return PREDEFINED_CONSTANT_NAME[type];
}

PredefinedConstantType PredefinedConstant::NameToType(stringid name) {
  for (int i = 0; i < PREDEFINED_CONSTANT_COUNT; ++i) {
    if (name == PREDEFINED_CONSTANT_NAME[i]) return (PredefinedConstantType)i;
  }
  return PREDEFINED_CONSTANT_COUNT;
}
