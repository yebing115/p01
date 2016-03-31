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
  JOB_AFFINITY_RENDER,     // run on render thread.
  NUM_JOB_AFFINITIES,
};

struct Job {
  typedef void(*JobFn)(void* user_data);
  JobFn* _fn;
  void* _user_data;
  JobPriority _priority;
  JobAffinity _affinity;
};
