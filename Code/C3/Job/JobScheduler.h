#pragma once
#include "Data/C3Data.h"
#include "Platform/C3Platform.h"
#include "Pattern/Singleton.h"
#include "Job.h"

struct JobNode {
  Job _job;
  Fiber* _fiber;
  Handle _wait_handle;
  list_head _wait_link;
};

struct JobWaitingList {
  Fiber* _fiber; // suspended
  SpinLock _spin_lock;
  atomic_int _num;
  list_head _waiting_job_nodes;
};

class JobScheduler {
public:
  JobScheduler();
  ~JobScheduler();

  Handle Submit(Job* job, int num_jobs = 1);
  void WaitFor(Handle handle);
  JobNode* GetNext();
  void Finish(JobNode* job_node);

private:
  SpinLock _waiting_list_lock;
  HandleAlloc<C3_MAX_JOBS, true> _wait_handle_alloc;
  JobWaitingList _waiting_lists[C3_MAX_JOBS];
  ThreadSafePoolAllocator _job_node_allocator;
  Fiber _fibers[C3_MAX_FIBERS];
  SpinLock _fiber_lock;
  u16 _num_free_fibers;
  u16 _free_fiber_index[C3_MAX_FIBERS];
  mpmc_bounded_queue_t<JobNode*> _job_queue;
  mpmc_bounded_queue_t<JobNode*> _wait_fiber_queue;
  SUPPORT_SINGLETON(JobScheduler);
};
