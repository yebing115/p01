#pragma once
#include "Data/C3Data.h"
#include "Platform/C3Platform.h"
#include "Pattern/Singleton.h"
#include "Job.h"

struct JobNode {
  JobFn _fn;
  void* _user_data;
  JobPriority _priority;
  JobAffinity _affinity;
  Fiber* _fiber;
  atomic_int* _label;
  list_head _link;
  int _reschedule;
};
typedef MPMCQueue<JobNode> JobQueue;

struct JobWaitListNode {
  SpinLock _lock;
  atomic_int _label;
  int _wait_value;
  list_head _job_list;
  list_head _link;
};

class JobScheduler {
public:
  JobScheduler();
  ~JobScheduler();

  void Init(int num_workers);
  atomic_int* SubmitJobs(Job* start_job, int num_jobs);
  void WaitAndFreeJobs(atomic_int* label) { WaitJobs(label, true); }
  void WaitJobs(atomic_int* label) { WaitJobs(label, false); }
  void WaitCounter(atomic_int* external_label, int value);
  void Yield();

private:
  void AddJob(JobNode* job_node);
  void DoJob(JobNode* job_node);
  void WaitJobs(atomic_int* label, bool free_wait_list);
  JobNode* GetJob(JobAffinity affinity, JobPriority priority);
  static void MainScheduleFiber(void* arg);
  static void DoJobFiber(void* arg);
  static i32 WorkerThread(void* arg);
  SpinLock _job_queues_lock;
  list_head _job_queues[NUM_JOB_AFFINITIES][NUM_JOB_PRIORITIES];
  Thread _worker_threads[C3_MAX_WORKER_THREADS];
  int _num_workers;
  int _require_exit;
  typedef ThreadSafePool<Fiber, C3_MAX_FIBERS> FiberPool;
  FiberPool* _fiber_pool;
  SpinLock _wait_lock;
  list_head _wait_list;
  PoolAllocator _wait_allocator;
  ThreadSafePoolAllocator _job_allocator;
  JobNode* _root_job;
  SUPPORT_SINGLETON(JobScheduler);
};
