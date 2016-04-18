#include "C3PCH.h"
#include "SimpleFileLoader.h"

DEFINE_JOB_ENTRY(load_file) {
  auto asset = (Asset*)arg;
  auto f = FileSystem::Instance()->OpenRead(asset->_desc._filename);
  if (!f) {
    asset->_state = ASSET_STATE_EMPTY;
    return;
  }
  SpinLockGuard lock_guard(&asset->_lock);
  asset->_header = (AssetMemoryHeader*)C3_ALLOC(g_allocator, ASSET_MEMORY_SIZE(0, f->GetSize()));
  asset->_header->_size = ASSET_MEMORY_SIZE(0, f->GetSize());
  asset->_header->_num_depends = 0;
  auto data = asset->_header->GetData();
  f->ReadBytes(data, f->GetSize());
  FileSystem::Instance()->Close(f);
  asset->_state = ASSET_STATE_READY;
}

static atomic_int* file_load_async(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_LOADING);
  auto JS = JobScheduler::Instance();
  Job job;
  job.InitWorkerJob(load_file, asset);
  return JS->SubmitJobs(&job, 1);
}

static void file_unload(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_UNLOADING);
}

AssetOperations SIMPLE_FILE_OPS = {
  &file_load_async,
  &file_unload,
};
