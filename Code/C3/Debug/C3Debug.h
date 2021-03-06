#pragma once

#include "LogManager.h"
#include "Logger.h"
#include "LogConfig.h"
#include <assert.h>

#ifdef NO_LOG_MANAGER
#define c3_log(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define c3_log(fmt, ...) LogManager::Instance()->Log(fmt, ##__VA_ARGS__)
#endif
#define c3_assert assert
#define c3_assert_return(Expression) \
  if (!(Expression)) { \
    c3_log("%s:%d Assert: %s\n", __FILE__, __LINE__, #Expression); \
    return; \
  }
#define c3_assert_return_x(Expression, Ret) \
  if (!(Expression)) { \
    c3_log("%s:%d Assert: %s\n", __FILE__, __LINE__, #Expression); \
    return (Ret); \
      }
