#pragma once
#include "Platform/C3Platform.h"

enum JobPriority {
  JOB_PRIORITY_LOW,
  JOB_PRIORITY_NORMAL,
  JOB_PRIORITY_HIGH,
};

enum JobFlag {
  JOB_FLAG_DEFAULT = 0,           // run on any threads.
  JOB_FLAG_MAIN = (1 << 0),       // run on main thread.
  JOB_FLAG_RENDER = (1 << 1),     // run on render thread.
  JOB_FLAG_BACKGROUND = (1 << 2), // run on worker threads.
};

struct Job {
  typedef void(*JobFn)(void* user_data);
  JobFn* _fn;
  void* _user_data;
  JobPriority _prio;
  int _flags;
};
