#pragma once
#include "Platform/C3Platform.h"

enum JobPriority {
  JOB_PRIORITY_HIGH,
  JOB_PRIORITY_NORMAL,
  JOB_PRIORITY_LOW,
  NUM_JOB_PRIORITIES,
};

enum JobAffinity {
  JOB_AFFINITY_ANY,        // run on any threads.
  JOB_AFFINITY_MAIN,       // run on main thread.
  NUM_JOB_AFFINITIES,
};

typedef void(*JobFn)(void* arg);
struct Job {
  JobFn _fn;
  void* _user_data;
  JobPriority _priority;
  JobAffinity _affinity;
};

#define DEFINE_JOB_ENTRY(name) void name(void* arg)
#define DEFINE_JOB_LAMBDA(name) JobFn name = [](void* arg)