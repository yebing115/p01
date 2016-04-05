#include "C3PCH.h"
#include "DDSTextureLoader.h"

struct CreateTextureParam {
  Texture* _texture;
  const MemoryBlock* _mem;
};

DEFINE_JOB_ENTRY(create_texture) {
  auto param = (CreateTextureParam*)arg;
  auto GR = GraphicsRenderer::Instance();
  param->_texture->_handle = GR->CreateTexture(param->_mem, C3_TEXTURE_NONE, 0,
                                               &param->_texture->_info);
  c3_log("Created texture handle: %d\n", param->_texture->_handle.idx);
}

DEFINE_JOB_ENTRY(load_dds_texture) {
  auto asset = (Asset*)arg;
  auto f = FileSystem::Instance()->OpenRead(asset->_desc._filename);
  if (!f) {
    asset->_state = ASSET_STATE_EMPTY;
    return;
  }
  auto mem = mem_alloc(f->GetSize());
  f->ReadBytes(mem->data, mem->size);
  FileSystem::Instance()->Close(f);

  SpinLockGuard lock_guard(&asset->_lock);
  asset->_header = (AssetMemoryHeader*)C3_ALLOC(g_allocator, ASSET_MEMORY_SIZE(0, sizeof(Texture)));
  asset->_header->_size = ASSET_MEMORY_SIZE(0, sizeof(Texture));
  asset->_header->_num_depends = 0;
  auto texture = (Texture*)asset->_header->GetData();
  texture->_mem = mem;

  CreateTextureParam param;
  param._texture = (Texture*)asset->_header->GetData();
  param._mem = mem;

  auto JS = JobScheduler::Instance();
  Job job;
  job.InitMainJob(create_texture, &param);
  JS->WaitAndFreeJobs(JS->SubmitJobs(&job, 1));

  asset->_state = ASSET_STATE_READY;
}

static atomic_int* dds_load_async(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_LOADING);
  auto JS = JobScheduler::Instance();
  Job job;
  job.InitWorkerJob(load_dds_texture, asset);
  return JS->SubmitJobs(&job, 1);
}

static void dds_unload(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_UNLOADING);
}

AssetOperations DDS_TEXTURE_OPS = {
  &dds_load_async,
  &dds_unload,
};
