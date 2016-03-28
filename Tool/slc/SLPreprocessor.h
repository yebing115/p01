#pragma once

#include "Data/DataType.h"
#include "Data/String.h"
#include <fpp.h>

#define MAX_FPP_TAGS 256
#define MAX_SCRATCH 4096

class SLPreprocessor {
public:
  SLPreprocessor();
  ~SLPreprocessor();

  bool SetFilename(const String& filename);
  void AddDefine(const String& define);
  bool Run();
  String GetResult() const;
  void Reset();

  static char* OnInput(char *buffer, int size, void *userdata);
  static void OnOutput(int c, void *userdata);
  static void OnError(void* userdata, char* format, va_list ap);

private:
  char* Scratch(const String& s);

  String _filename;
  FILE* _f;
  String _result;
  fppTag _tags[MAX_FPP_TAGS];
  int _num_tags;
  char _scratch[MAX_SCRATCH];
  int _num_scratch;
};
