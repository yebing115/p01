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
  JobNode* GetNext();
  void Finish(JobNode* job_node);

private:
  void DoJob();
  JobQueue* _job_queues[NUM_JOB_AFFINITIES][NUM_JOB_PRIORITIES];
  int _num_workers;
  SUPPORT_SINGLETON(JobScheduler);
};
