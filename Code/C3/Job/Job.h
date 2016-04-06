#pragma once
#include "Platform/C3Platform.h"

enum JobType {
  JOB_TYPE_WORKER,      // run on any threads.
  JOB_TYPE_MAIN,        // run on main thread.
  NUM_JOB_TYPES,
};

typedef FiberFn JobFn;
struct Job {
  JobFn _fn;
  void* _user_data;
  JobType _type;

  void Init(JobFn fn, void* user_data, JobType type) {
    _fn = fn;
    _user_data = user_data;
    _type = type;
    _type = JOB_TYPE_WORKER;
  }
  void InitWorkerJob(JobFn fn, void* user_data) { Init(fn, user_data, JOB_TYPE_WORKER); }
  void InitMainJob(JobFn fn, void* user_data) { Init(fn, user_data, JOB_TYPE_MAIN); }
};

#define DEFINE_JOB_ENTRY(name) void name(void* arg)
#define DEFINE_JOB_LAMBDA(name) JobFn name = [](void* arg)