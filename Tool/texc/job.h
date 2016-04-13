#pragma once

#include <stdint.h>
#include "mpmc-bounded-queue.hpp"

struct Job {
  const uint8_t* ibuf;
  uint8_t* obuf;
  int pitch;
  int mask;
  int flags;

  Job() {}
  Job(const uint8_t* i_buf, int pitch_, uint8_t* o_buf, int f, int m = 0xffffffff)
  : ibuf(i_buf), pitch(pitch_), obuf(o_buf), flags(f), mask(m) {}
};


typedef mpmc_bounded_queue_t<Job> JobQueue;