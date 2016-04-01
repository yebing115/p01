#include "C3PCH.h"
#include "ThreadAffinity.h"

static thread::id g_worker_thread_id[C3_MAX_WORKER_THREADS];

namespace ThreadAffinity {
bool IsMainThread() {
  return std::this_thread::get_id() == g_worker_thread_id[0];
}
int GetWorkerThreadIndex() {
  auto id = std::this_thread::get_id();
  for (int i = 0; i < C3_MAX_WORKER_THREADS; ++i) {
    if (id == g_worker_thread_id[i]) return i;
  }
  c3_log("Failed to get worker thread index.\n");
  return 0;
}
void RegisterWorkerThread(int worker_index) {
  if (worker_index >= 0 && worker_index < C3_MAX_WORKER_THREADS) {
    g_worker_thread_id[worker_index] = std::this_thread::get_id();
    atomic_thread_fence(memory_order_seq_cst);
  }
}
}


