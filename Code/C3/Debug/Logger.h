#pragma once

#include "Platform/PlatformConfig.h"
#include "Data/String.h"

class Logger {
public:
  virtual ~Logger() {}
  virtual void Log(const char* /*text*/) {}
};

class StdoutLogger : public Logger {
public:
  void Log(const char* text) override;
};

class FileLogger : public Logger {
public:
  FileLogger(const String& filename);
  ~FileLogger();
  void Log(const char* text) override;
private:
  FILE* _file;
};

#if ON_WINDOWS
class VSDebugLogger : public Logger {
public:
  void Log(const char* text) override;
};
#define DebugLogger VSDebugLogger
#else
#define DebugLogger StdoutLogger
#endif
