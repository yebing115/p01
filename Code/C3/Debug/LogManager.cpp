#include "C3PCH.h"
#include "LogManager.h"
#include "Logger.h"
#include <stdarg.h>

DEFINE_SINGLETON_INSTANCE(LogManager);

static thread_local char s_buf[32768];

void LogManager::Log(const char* format, ...) const {
  va_list ap;
  va_start(ap, format);
  _vsnprintf(s_buf, sizeof(s_buf), format, ap);
  va_end(ap);

  for (auto logger : _loggers) logger->Log(s_buf);
}