#include "C3PCH.h"
#include "JobScheduler.h"

DEFINE_SINGLETON_INSTANCE(JobScheduler);

JobScheduler::JobScheduler(): _job_queue(C3_MAX_FIBERS), _wait_fiber_queue(C3_MAX_JOBS) {
  memset(_waiting_lists, 0, sizeof(_waiting_lists));
  size_t obj_size = ALIGN_MASK(sizeof(JobNode), CACHELINE_SIZE - 1);
  size_t pool_size = C3_MAX_JOBS * obj_size;
  _job_node_allocator.Init(C3_ALLOC(g_allocator, pool_size), pool_size, obj_size);
  _waiting_list_lock.Reset();
  
  _fiber_lock.Reset();
  _num_free_fibers = C3_MAX_FIBERS;
  for (u16 i = 0; i < _num_free_fibers; ++i) _free_fiber_index[i] = i;
}

JobScheduler::~JobScheduler() {
  C3_FREE(g_allocator, _job_node_allocator.GetPool());
}

Handle JobScheduler::Submit(Job *first_job, int num_jobs) {
  Handle h = _wait_handle_alloc.Alloc();
  if (h) {
    auto& wait_node = _waiting_lists[h.idx];
    wait_node._fiber = nullptr;
    wait_node._num = num_jobs;
    INIT_LIST_HEAD(&wait_node._waiting_job_nodes);
    _fiber_lock.Lock();
    for (auto job = first_job; job < first_job + num_jobs; ++job) {
      JobNode* job_node = C3_NEW(&_job_node_allocator, JobNode);
      memcpy(&job_node->_job, job, sizeof(Job));
      job_node->_fiber = _num_free_fibers > 0 ? &_fibers[--_num_free_fibers] : nullptr;
      job_node->_wait_handle = h;
      list_add_tail(&job_node->_wait_link, &wait_node._waiting_job_nodes);
      if (job_node->_fiber) {
        while (!_job_queue.enqueue(job_node))
          ;
      } else {
        while (!_wait_fiber_queue.enqueue(job_node))
          ;
      }
    }
    _fiber_lock.Unlock();
  }
  return h;
}

void JobScheduler::WaitFor(Handle handle) {
  if (!_wait_handle_alloc.IsValid(handle)) {
    c3_log("[C3] wait for invalid job handle.\n");
    return;
  }
  auto& wait_node = _waiting_lists[handle.idx];
  while (wait_node._num.load() > 0) {
    JobNode* job_node = GetNext();
    if (job_node) {
      auto self = Fiber::GetCurrentFiber();
      self->Suspend();
      wait_node._spin_lock.Lock();
      wait_node._fiber = self;
      wait_node._spin_lock.Unlock();
      job_node->_fiber->Resume();
    } else {
      std::this_thread::yield();
    }
  }
  _wait_handle_alloc.Free(handle);
}

JobNode* JobScheduler::GetNext() {
  JobNode* job_node = nullptr;
  _job_queue.dequeue(job_node);
  return job_node;
}

void JobScheduler::Finish(JobNode* job_node) {
  auto& wait_node = _waiting_lists[job_node->_wait_handle.idx];
  if (wait_node._num.fetch_sub() == 1) {
    wait_node._spin_lock.Lock();
    list_del(&job_node->_wait_link);
    if (wait_node._fiber) {
       JobNode* job_node = wait_node._fiber->GetData();
       while (!_job_queue.enqueue(job_node))
         ;
    }
  }
  wait_node._spin_lock.Unlock();
}
