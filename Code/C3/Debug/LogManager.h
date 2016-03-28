#pragma once
#include "Data/DataType.h"
#include "Pattern/Singleton.h"

class Logger;
class LogManager {
public:
  void AddLogger(Logger* logger) { _loggers.push_back(logger); }
  void Log(const char* format, ...) const;
private:
  vector<Logger*> _loggers;
  SUPPORT_SINGLETON(LogManager);
};
