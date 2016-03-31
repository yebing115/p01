#include "C3PCH.h"
#include "JobScheduler.h"

DEFINE_SINGLETON_INSTANCE(JobScheduler);

JobScheduler::JobScheduler(int num_workers) {
  _num_workers = min<int>(C3_MAX_WORKER_THREADS, num_workers);
  _require_exit = 0;
  for (int i = 0; i < _num_workers; ++i) {
    _worker_threads[i].Init(&JobScheduler::WorkerThread, (void*)i);
  }
}

JobScheduler::~JobScheduler() {}

void JobScheduler::Submit(Job* start_job, int num_jobs, atomic_int* label) {
  if (label) label->store(num_jobs);
  JobNode job_node;
  job_node._label = label;
  for (Job* job = start_job; job < start_job + num_jobs; ++job) {
    job_node._job = *job;
    auto q = _job_queues[job->_affinity][job->_priority];
    while (!q->Write(job_node)) DoJob();
  }
  
}

void JobScheduler::Wait(atomic_int* label, int value) {
  while (label->load() != value) DoJob();
}

void JobScheduler::DoJob() {
  JobNode job_node;
  if (g_thread_id == MAIN_THREAD_ID) {
    if (GetJob(JOB_AFFINITY_MAIN, JOB_PRIORITY_HIGH, job_node)) goto found_job;
    if (GetJob(JOB_AFFINITY_MAIN, JOB_PRIORITY_NORMAL, job_node)) goto found_job;
    if (GetJob(JOB_AFFINITY_MAIN, JOB_PRIORITY_LOW, job_node)) goto found_job;
  }
  if (g_thread_id == RENDER_THREAD_ID) {
    if (GetJob(JOB_AFFINITY_RENDER, JOB_PRIORITY_HIGH, job_node)) goto found_job;
    if (GetJob(JOB_AFFINITY_RENDER, JOB_PRIORITY_NORMAL, job_node)) goto found_job;
    if (GetJob(JOB_AFFINITY_RENDER, JOB_PRIORITY_LOW, job_node)) goto found_job;
  }
  if (GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_HIGH, job_node)) goto found_job;
  if (GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_NORMAL, job_node)) goto found_job;
  if (GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_LOW, job_node)) goto found_job;
  std::this_thread::yield();
  return;
found_job:
  DoJob(job_node);
}

void JobScheduler::DoJob(const JobNode& job_node) {
  (*job_node._job._fn)(job_node._job._user_data);
  if (job_node._label) --(*job_node._label);
}

i32 JobScheduler::WorkerThread(void* arg) {
  int index = (int)arg;
  g_thread_id = WORKER_THREAD_ID(index);
  auto JS = JobScheduler::Instance();
  JobNode job_node;
  while (!JS->_require_exit) {
    if (JS->GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_HIGH, job_node) ||
        JS->GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_NORMAL, job_node) ||
        JS->GetJob(JOB_AFFINITY_ANY, JOB_PRIORITY_LOW, job_node)) {
      JS->DoJob(job_node);
    } else std::this_thread::yield();
  }
  return 0;
}
