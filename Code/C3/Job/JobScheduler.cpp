#include "C3PCH.h"
#include "JobScheduler.h"
#include "ThreadAffinity.h"
using namespace ThreadAffinity;

DEFINE_SINGLETON_INSTANCE(JobScheduler);

JobScheduler::JobScheduler() {
  _num_workers = 0;
  _require_exit = 0;
  _wait_allocator.Init(sizeof(JobWaitListNode), ALIGN_OF(JobWaitListNode), C3_MAX_JOBS, g_allocator);
  _job_allocator.Init(sizeof(JobNode), ALIGN_OF(JobNode), C3_MAX_JOBS, g_allocator);
  for (int aff = 0; aff < NUM_JOB_AFFINITIES; ++aff) {
    for (int prio = 0; prio < NUM_JOB_PRIORITIES; ++prio) {
      INIT_LIST_HEAD(&_job_queues[aff][prio]);
    }
  }
  INIT_LIST_HEAD(&_wait_list);

  _main_sched_job = C3_NEW(&_job_allocator, JobNode);
  memset(_main_sched_job, 0, sizeof(JobNode));
  _main_sched_job->_affinity = JOB_AFFINITY_MAIN;
  _main_sched_job->_priority = JOB_PRIORITY_NORMAL;
  INIT_LIST_HEAD(&_main_sched_job->_link);
  _main_sched_job->_fiber = Fiber::ConvertFromThread(&_main_sched_job);
  _fiber_pool = C3_NEW(g_allocator, FiberPool);
}

JobScheduler::~JobScheduler() {}

void JobScheduler::Init(int num_workers) {
  RegisterWorkerThread(0);
  _num_workers = min<int>(C3_MAX_WORKER_THREADS, num_workers);
  for (int i = 1; i < _num_workers; ++i) {
    char thread_name[128];
    snprintf(thread_name, sizeof(thread_name), "Worker%d", i);
    _worker_threads[i].Init(&JobScheduler::WorkerThread, (void*)i, 0, thread_name, i);
  }
}

void JobScheduler::Submit(Job* start_job, int num_jobs, atomic_int** label) {
  if (num_jobs <= 0) return;
  _wait_lock.Lock();
  JobWaitListNode* wait_list = C3_NEW(&_wait_allocator, JobWaitListNode);
  wait_list->_label = num_jobs;
  *label = &wait_list->_label;
  INIT_LIST_HEAD(&wait_list->_job_list);
  list_add_tail(&wait_list->_link, &_wait_list);
  _wait_lock.Unlock();
  JobNode* job_node;
  for (Job* job = start_job; job < start_job + num_jobs; ++job) {
    job_node = C3_NEW(&_job_allocator, JobNode);
    job_node->_fn = job->_fn;
    job_node->_user_data = job->_user_data;
    job_node->_affinity = job->_affinity;
    job_node->_priority = job->_priority;
    job_node->_fiber = nullptr;
    job_node->_label = &wait_list->_label;
    INIT_LIST_HEAD(&job_node->_link);
    AddJob(job_node);
  }
}

void JobScheduler::Wait(atomic_int* label, int value, bool free_wait_list) {
  JobWaitListNode* wait_list = container_of(label, JobWaitListNode, _label);
  if (*label > value) {
    auto self = Fiber::GetCurrentFiber();
    auto job_node = (JobNode*)self->GetData();
    job_node->_wait_value = value;
    wait_list->_lock.Lock();
    list_add_tail(&job_node->_link, &wait_list->_job_list);
    wait_list->_lock.Unlock();
    if (IsMainThread()) ScheduleMain(label, value);
    else self->Suspend();
  }
  if (free_wait_list) {
    _wait_lock.Lock();
    list_del(&wait_list->_link);
    C3_DELETE(&_wait_allocator, wait_list);
    _wait_lock.Unlock();
  }
}

JobNode* JobScheduler::GetJob(JobAffinity affinity, JobPriority priority) {
  SpinLockGuard lock_guard(&_job_queues_lock);
  auto& l = _job_queues[affinity][priority];
  if (list_empty(&l)) return nullptr;
  JobNode* job_node = list_first_entry(&l, JobNode, _link);
  //c3_log("GetJob %p\n", job_node);
  list_del(&job_node->_link);
  return job_node;
}

void JobScheduler::ScheduleMain(atomic_int* label, int value) {
  JobNode* job_node;
retry:
  if (job_node = GetJob(JOB_AFFINITY_MAIN, JOB_PRIORITY_HIGH)) goto found_job;
  if (job_node = GetJob(JOB_AFFINITY_MAIN, JOB_PRIORITY_NORMAL)) goto found_job;
  if (job_node = GetJob(JOB_AFFINITY_MAIN, JOB_PRIORITY_LOW)) goto found_job;

  if (job_node = GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_HIGH)) goto found_job;
  if (job_node = GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_NORMAL)) goto found_job;
  if (job_node = GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_LOW)) goto found_job;
  std::this_thread::yield();
  goto retry;
found_job:
  DoJob(job_node);
  if (*label > value) goto retry;
}

void JobScheduler::AddJob(JobNode* job_node) {
  SpinLockGuard lock_guard(&_job_queues_lock);
  auto& job_list = _job_queues[job_node->_affinity][job_node->_priority];
  //c3_log("AddJob %p\n", job_node);
  list_add_tail(&job_node->_link, &job_list);
}

void JobScheduler::DoJob() {
  JobNode* job_node;
  if (IsMainThread()) {
    if (job_node = GetJob(JOB_AFFINITY_MAIN, JOB_PRIORITY_HIGH)) goto found_job;
    if (job_node = GetJob(JOB_AFFINITY_MAIN, JOB_PRIORITY_NORMAL)) goto found_job;
    if (job_node = GetJob(JOB_AFFINITY_MAIN, JOB_PRIORITY_LOW)) goto found_job;
  }
  if (job_node = GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_HIGH)) goto found_job;
  if (job_node = GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_NORMAL)) goto found_job;
  if (job_node = GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_LOW)) goto found_job;
  std::this_thread::yield();
  return;
found_job:
  DoJob(job_node);
}

void JobScheduler::DoJob(JobNode* job_node) {
  job_node->_fn(job_node->_user_data);
  job_node->_label->fetch_sub(1);
}

i32 JobScheduler::WorkerThread(void* arg) {
  RegisterWorkerThread((int)arg);

  i32(*fiber_fn)(void*) = [](void* user_data) -> i32 {
    auto JS = JobScheduler::Instance();
    JS->DoJob((JobNode*)user_data);
    return 0;
  };

  auto JS = JobScheduler::Instance();
  JobNode self_job;
  auto sched_fiber = Fiber::ConvertFromThread(&self_job);
  JobNode* job_node = nullptr;
  while (!JS->_require_exit) {
    if ((job_node = JS->GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_HIGH)) ||
        (job_node = JS->GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_NORMAL)) ||
        (job_node = JS->GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_LOW))) {
      //c3_log("run job %p\n", job_node);
      Fiber* fiber = JS->_fiber_pool->GetWait();
      sched_fiber->Suspend();
      fiber->Prepare(fiber_fn, job_node);
      fiber->Resume();
      
      auto fiber_state = fiber->GetState();
      if (fiber_state == FIBER_STATE_FINISHED) {
        //c3_log("finish job %p\n", job_node);
        auto cur_value = job_node->_label->fetch_sub(1) - 1;
        JobWaitListNode* wait_list = container_of(job_node->_label, JobWaitListNode, _label);
        JobNode* job_wake, *tmp;
        wait_list->_lock.Lock();
        list_for_each_entry_safe(job_wake, tmp, &wait_list->_job_list, _link) {
          if (cur_value <= job_wake->_wait_value) {
            list_del(&job_wake->_link);
            JS->AddJob(job_wake);
          }
        }
        wait_list->_lock.Unlock();
        C3_DELETE(&JS->_job_allocator, job_node);
      } else if (fiber_state == FIBER_STATE_SUSPENDED) {
        // do nothing...
      }
      job_node = nullptr;
    } else std::this_thread::yield();
  }
  return 0;
}
