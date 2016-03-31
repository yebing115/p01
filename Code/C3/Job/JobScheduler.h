#pragma once
#include "Data/C3Data.h"
#include "Platform/C3Platform.h"
#include "Pattern/Singleton.h"
#include "Job.h"

struct JobNode {
  Job _job;
  atomic_int* _label;
};
typedef MPMCQueue<JobNode> JobQueue;

class JobScheduler {
public:
  JobScheduler(int num_workers = thread::hardware_concurrency());
  ~JobScheduler();

  void Submit(Job* start_job, int num_jobs = 1, atomic_int* label = nullptr);
  void Wait(atomic_int* label, int value = 0);

private:
  void DoJob();
  void DoJob(const JobNode& job_node);
  inline bool GetJob(JobAffinity affinity, JobPriority priority, JobNode& out_job_node) {
    return _job_queues[affinity][priority]->Read(out_job_node);
  }
  static i32 WorkerThread(void* arg);
  JobQueue* _job_queues[NUM_JOB_AFFINITIES][NUM_JOB_PRIORITIES];
  Thread _worker_threads[C3_MAX_WORKER_THREADS];
  int _num_workers;
  int _require_exit;
  SUPPORT_SINGLETON(JobScheduler);
};
