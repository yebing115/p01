#include "C3PCH.h"
#include "MEXModelLoader.h"

struct CreateModelParam {
  Model* _model;
  MeshHeader* _header;
  VertexDecl _decl;
  const MemoryRegion* _vb_mem;
  const MemoryRegion* _ib_mem;
};

DEFINE_JOB_ENTRY(load_mex_model) {
  auto asset = (Asset*)arg;
  auto f = FileSystem::Instance()->OpenRead(asset->_desc._filename);
  if (!f) {
    asset->_state = ASSET_STATE_EMPTY;
    return;
  }

  MeshHeader header;
  f->ReadBytes(&header, sizeof(header));

  SpinLockGuard lock_guard(&asset->_lock);
  auto model_size = Model::ComputeSize(header.num_parts);
  asset->_header = (AssetMemoryHeader*)C3_ALLOC(g_allocator, ASSET_MEMORY_SIZE(0, model_size));
  asset->_header->_size = ASSET_MEMORY_SIZE(0, model_size);
  asset->_header->_num_depends = 0;
  auto model = (Model*)asset->_header->GetData();

  model->_filename = String::GetID(asset->_desc._filename);
  model->_aabb = header.aabb;
  model->_num_parts = header.num_parts;
  ModelPart* part = model->_parts;
  ModelPart* part_end = model->_parts + header.num_parts;
  MeshPart mesh_part;
  f->Seek(header.part_data_offset);
  for (; part < part_end; ++part) {
    f->ReadBytes(&mesh_part, sizeof(MeshPart));
    part->_start_index = mesh_part.start_index;
    part->_num_indices = mesh_part.num_indices;
    part->_aabb = mesh_part.aabb;
  }

  CreateModelParam param;
  auto vb_mem = mem_alloc(header.num_vertices * header.vertex_stride);
  int index_size = header.num_indices >= 0x10000 ? 4 : 2;
  auto ib_mem = mem_alloc(header.num_indices * index_size);
  f->Seek(header.vertex_data_offset);
  f->ReadBytes(vb_mem->data, vb_mem->size);
  f->Seek(header.index_data_offset);
  f->ReadBytes(ib_mem->data, ib_mem->size);
  FileSystem::Instance()->Close(f);

  VertexDecl vd;
  vd.Begin();
  for (int i = 0; i < header.num_attrs; ++i) {
    MeshAttr& attr = header.attrs[i];
    vd.Add((VertexAttr)attr.attr, attr.num, (DataType)attr.data_type,
           attr.flags & MESH_ATTR_NORMALIZED, attr.flags & MESH_ATTR_AS_INT);
  }
  vd.End();

  auto GR = GraphicsRenderer::Instance();
  model->_vb = GR->CreateVertexBuffer(vb_mem, vd);
  u32 ib_flags = header.num_indices >= 0x10000 ? C3_BUFFER_INDEX32 : C3_BUFFER_NONE;
  model->_ib = GR->CreateIndexBuffer(ib_mem, ib_flags);

  asset->_state = ASSET_STATE_READY;
}

static atomic_int* mex_load_async(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_LOADING);
  auto JS = JobScheduler::Instance();
  Job job;
  job.InitWorkerJob(load_mex_model, asset);
  return JS->SubmitJobs(&job, 1);
}

static void mex_unload(Asset* asset) {}

AssetOperations MEX_MODEL_OPS = {
  &mex_load_async,
  &mex_unload,
};
