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
  for (int t = 0; t < NUM_JOB_TYPES; ++t) {
    INIT_LIST_HEAD(&_job_queues[t]);
  }
  INIT_LIST_HEAD(&_wait_list);

  _fiber_pool = C3_NEW(g_allocator, FiberPool);
  _root_job = C3_NEW(&_job_allocator, JobNode);
  memset(_root_job, 0, sizeof(JobNode));
  _root_job->_type = JOB_TYPE_MAIN;
  _root_job->_reschedule = false;
  INIT_LIST_HEAD(&_root_job->_link);
  _root_job->_fiber = Fiber::ConvertFromThread(_root_job);
  //c3_log("root job %p\n", _root_job);
}

JobScheduler::~JobScheduler() {}

void JobScheduler::Init(int num_workers) {
  RegisterWorkerThread(0);
  auto sched_fiber = _fiber_pool->GetWait();
  sched_fiber->Prepare(&JobScheduler::MainScheduleFiber, sched_fiber);
  _root_job->_fiber->SetState(FIBER_STATE_SUSPENDED);
  sched_fiber->Resume();

  _num_workers = min<int>(C3_MAX_WORKER_THREADS, num_workers);
  for (int i = 1; i < _num_workers; ++i) {
    char thread_name[128];
    snprintf(thread_name, sizeof(thread_name), "Worker%d", i);
    _worker_threads[i].Init(&JobScheduler::WorkerThread, (void*)i, 0, thread_name, i);
  }
}

atomic_int* JobScheduler::SubmitJobs(Job* start_job, int num_jobs) {
  if (num_jobs <= 0) return nullptr;
  _wait_lock.Lock();
  JobWaitListNode* wait_list = C3_NEW(&_wait_allocator, JobWaitListNode);
  wait_list->_label = num_jobs;
  wait_list->_wait_value = 0;
  INIT_LIST_HEAD(&wait_list->_job_list);
  list_add_tail(&wait_list->_link, &_wait_list);
  _wait_lock.Unlock();
  JobNode* job_node;
  for (Job* job = start_job; job < start_job + num_jobs; ++job) {
    job_node = C3_NEW(&_job_allocator, JobNode);
    job_node->_fn = job->_fn;
    job_node->_user_data = job->_user_data;
    job_node->_type = job->_type;
    job_node->_fiber = nullptr;
    job_node->_label = &wait_list->_label;
    job_node->_reschedule = false;
    INIT_LIST_HEAD(&job_node->_link);
    AddJob(job_node);
  }
  return &wait_list->_label;
}

void JobScheduler::WaitJobs(atomic_int* label, bool free_wait_list) {
  if (!label) return;
  JobWaitListNode* wait_list = container_of(label, JobWaitListNode, _label);
  wait_list->_lock.Lock();
  if (*label != wait_list->_wait_value) {
    auto self = Fiber::GetCurrentFiber();
    auto job_node = (JobNode*)self->GetData();
    list_add_tail(&job_node->_link, &wait_list->_job_list);
    wait_list->_lock.Unlock();
    //c3_log("%d: %p waiting \n", GetWorkerThreadIndex(), job_node);
    self->Suspend();
  } else wait_list->_lock.Unlock();
  if (free_wait_list) {
    _wait_lock.Lock();
    list_del(&wait_list->_link);
    _wait_lock.Unlock();
    C3_DELETE(&_wait_allocator, wait_list);
  }
}

void JobScheduler::WaitCounter(atomic_int* label, int value) {
  while (*label != value) Yield();
}

void JobScheduler::Yield() {
  auto self = Fiber::GetCurrentFiber();
  self->SetState(FIBER_STATE_SUSPENDED);
  auto job_node = (JobNode*)self->GetData();
  job_node->_reschedule = true;
  self->Suspend();
}

void JobScheduler::FlushMainJobs() {
  _job_queues_lock.Lock();
  if (list_empty(&_job_queues[JOB_TYPE_MAIN])) {
    _job_queues_lock.Unlock();
    return;
  }
  _job_queues_lock.Unlock();
  auto self = Fiber::GetCurrentFiber();
  self->SetState(FIBER_STATE_SUSPENDED);
  auto job_node = (JobNode*)self->GetData();
  job_node->_type = JOB_TYPE_MAIN;
  job_node->_reschedule = true;
  self->Suspend();
}

void JobScheduler::FlushRenderJobs() {
  _job_queues_lock.Lock();
  if (list_empty(&_job_queues[JOB_TYPE_RENDER])) {
    _job_queues_lock.Unlock();
    return;
  }
  _job_queues_lock.Unlock();
  auto self = Fiber::GetCurrentFiber();
  self->SetState(FIBER_STATE_SUSPENDED);
  auto job_node = (JobNode*)self->GetData();
  job_node->_type = JOB_TYPE_RENDER;
  job_node->_reschedule = true;
  self->Suspend();
}

JobNode* JobScheduler::GetJob(JobType type) {
  SpinLockGuard lock_guard(&_job_queues_lock);
  auto& l = _job_queues[type];
  if (list_empty(&l)) return nullptr;
  JobNode* job_node = list_first_entry(&l, JobNode, _link);
  //c3_log("%d: GetJob %p\n", GetWorkerThreadIndex(), job_node);
  list_del_init(&job_node->_link);
  return job_node;
}

void JobScheduler::MainScheduleFiber(void* arg) {
  auto JS = JobScheduler::Instance();
  JobNode* self_job = C3_NEW(&JS->_job_allocator, JobNode);
  self_job->_fiber = (Fiber*)arg;
  Fiber::SetScheduleFiber(self_job->_fiber);
  self_job->_reschedule = false;
  //c3_log("0: self job %p\n", self_job);

  self_job->_fiber->SetState(FIBER_STATE_SUSPENDED);
  JS->_root_job->_fiber->Resume();

  JobNode* job_node = nullptr;
  while (!JS->_require_exit) {
    if ((job_node = JS->GetJob(JOB_TYPE_MAIN)) || 
        (job_node = JS->GetJob(JOB_TYPE_WORKER))) {
      JS->DoJob(job_node);
      job_node = nullptr;
    } else {
      //c3_log("%d: no job\n", (int)arg);
      std::this_thread::yield();
    }
  }
}

void JobScheduler::DoJobFiber(void* arg) {
  auto job_node = (JobNode*)arg;
  job_node->_fn(job_node->_user_data);
}

void JobScheduler::AddJob(JobNode* job_node) {
  SpinLockGuard lock_guard(&_job_queues_lock);
  auto& job_list = _job_queues[job_node->_type];
  //c3_log("%d: AddJob %p\n", GetWorkerThreadIndex(), job_node);
  list_add_tail(&job_node->_link, &job_list);
}

void JobScheduler::DoJob(JobNode* job_node) {
  auto sched_fiber = Fiber::GetCurrentFiber();
  sched_fiber->Suspend();
  if (!job_node->_fiber) {
    job_node->_fiber = _fiber_pool->GetWait();
    //c3_log("%d: run job %p, @%p\n", GetWorkerThreadIndex(), job_node, job_node->_fiber);
    job_node->_fiber->Prepare(&JobScheduler::DoJobFiber, job_node);
  }
  job_node->_fiber->Resume();

  auto fiber_state = job_node->_fiber->GetState();
  if (fiber_state == FIBER_STATE_FINISHED) {
    //c3_log("%d: finish job %p, @%p\n", GetWorkerThreadIndex(), job_node, job_node->_fiber);
    auto cur_value = --(*job_node->_label);
    JobWaitListNode* wait_list = container_of(job_node->_label, JobWaitListNode, _label);
    if (cur_value == wait_list->_wait_value) {
      JobNode* job_wake, *tmp;
      SpinLockGuard wait_guard(&wait_list->_lock);
      list_for_each_entry_safe(job_wake, tmp, &wait_list->_job_list, _link) {
        //c3_log("%d: wakeup job %p\n", GetWorkerThreadIndex(), job_wake);
        list_del_init(&job_wake->_link);
        AddJob(job_wake);
      }
    }
    _fiber_pool->Put(job_node->_fiber);
    job_node->_fiber = nullptr;
    C3_DELETE(&_job_allocator, job_node);
  } else if (fiber_state == FIBER_STATE_SUSPENDED) {
    if (job_node->_reschedule) {
      job_node->_reschedule = false;
      _fiber_pool->Put(job_node->_fiber);
      job_node->_fiber = nullptr;
      AddJob(job_node);
    }
    //c3_log("%d: yield job %p, state = %d\n", GetWorkerThreadIndex(), job_node, fiber_state);
  } else {
    //c3_log("%d: yield job %p, state = %d\n", GetWorkerThreadIndex(), job_node, fiber_state);
  }
}

i32 JobScheduler::WorkerThread(void* arg) {
  RegisterWorkerThread((int)arg);

  auto JS = JobScheduler::Instance();
  JobNode* self_job = C3_NEW(&JS->_job_allocator, JobNode);
  self_job->_fiber = Fiber::ConvertFromThread(self_job);
  self_job->_reschedule = false;
  //c3_log("%d: self job %p\n", (int)arg, self_job);
  JobNode* job_node = nullptr;
  while (!JS->_require_exit) {
    if (job_node = JS->GetJob(JOB_TYPE_WORKER)) {
      JS->DoJob(job_node);
      job_node = nullptr;
    } else {
      //c3_log("%d: no job\n", (int)arg);
      std::this_thread::yield();
    }
  }
  return 0;
}
