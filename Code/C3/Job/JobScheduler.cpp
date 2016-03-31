#include "C3PCH.h"
#include "JobScheduler.h"

DEFINE_SINGLETON_INSTANCE(JobScheduler);

JobScheduler::JobScheduler(int num_workers): _num_workers(num_workers) {}

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
    for (int prio = JOB_PRIORITY_HIGH; prio < NUM_JOB_AFFINITIES; ++prio) {
      auto q = _job_queues[JOB_AFFINITY_MAIN][prio];
      if (q->Read(job_node)) goto found_job;
    }
  } else if (g_thread_id == RENDER_THREAD_ID) {
    for (int prio = JOB_PRIORITY_HIGH; prio < NUM_JOB_AFFINITIES; ++prio) {
      auto q = _job_queues[JOB_AFFINITY_RENDER][prio];
      if (q->Read(job_node)) goto found_job;
    }
  }
  for (int prio = JOB_PRIORITY_HIGH; prio < NUM_JOB_AFFINITIES; ++prio) {
    auto q = _job_queues[JOB_AFFINITY_ANY][prio];
    if (q->Read(job_node)) goto found_job;
  }
  return;
found_job:
  (*job_node._job._fn)(job_node._job._user_data);
  if (job_node._label) --(*job_node._label);
}
