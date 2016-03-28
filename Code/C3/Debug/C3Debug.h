#pragma once

#include "LogManager.h"
#include "Logger.h"
#include "LogConfig.h"
#include <assert.h>

#define c3_log(fmt, ...) LogManager::Instance()->Log(fmt, ##__VA_ARGS__)
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
