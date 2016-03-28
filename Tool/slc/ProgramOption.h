#pragma once
#include "Data/DataType.h"
#include "Data/String.h"

enum ProgramOutputType {
  PROGRAM_OUTPUT_OPENGL,
  PROGRAM_OUTPUT_DIRECTX,
  PROGRAM_OUTPUT_PS4,
};

struct ProgramOption {
  vector<String> arguments;
  String input_filename;
  String output_filename;
  vector<String> defines;
  ProgramOutputType output_type;
  vector<String> pragmas;
  bool output_binary;
};

extern ProgramOption g_options;
