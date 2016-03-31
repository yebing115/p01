#include "C3PCH.h"
#include "DDSTextureLoader.h"

static void dds_load(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_LOADING);
  auto f = FileSystem::Instance()->OpenRead(asset->_desc._filename);
  if (!f) {
    asset->_state = ASSET_STATE_EMPTY;
    return;
  }
  asset->_header = (AssetMemoryHeader*)C3_ALLOC(g_allocator, ASSET_MEMORY_SIZE(0, sizeof(Texture)));
  auto mem = mem_alloc(f->GetSize());
  f->ReadBytes(mem->data, mem->size);
  FileSystem::Instance()->Close(f);
  Job::JobFn create_texture = [](void* user_data) {
    auto mem = (const MemoryBlock*)user_data;
  };
  Job job{
    &create_texture,
    (void*)mem,
    JOB_PRIORITY_LOW,
    JOB_AFFINITY_RENDER,
  };
  atomic_int label;
  JobScheduler::Instance()->Submit(&job, 1, &label);
}

static void dds_unload(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_UNLOADING);
}

AssetOperations DDS_TEXTURE_OPS = {
  &dds_load,
  &dds_unload,
};
