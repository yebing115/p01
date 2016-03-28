#include "SLParser.h"
#include "SLAnalyser.h"
#include "GLSLWriter.h"
#include "HLSLWriter.h"
#include "PSSLWriter.h"
#include "ProgramOption.h"
#include "Memory/MemoryBlock.h"
#include "write_helpers.h"

#pragma comment(lib, "fcpp.lib")

ProgramOption g_options;

void Usage() {
  fputs("slc [options] input_filename output_filename\n"
        "    options:\n"
        "        -b                Write Binary version.\n"
        "        -gl               Write OpenGL 3.x shader.\n"
        "        -dx               Write DirectX 11 shader.\n"
        "        -ps4              Write PS4 shader.\n"
        "        -DNAME            #define NAME\n"
        "        -DNAME=VALUE      #define NAME VALUE\n"
        "        -ps4-pragma XXXXX         add pragma\n", stderr);
}

bool ParseArguments(int argc, char* argv[]) {
  g_options.output_type = PROGRAM_OUTPUT_OPENGL;
  g_options.output_binary = false;
  g_options.arguments.resize(argc);
  for (int i = 0; i < argc; ++i) g_options.arguments[i].Set(argv[i]);
  
  if (argc < 3) return false;
  int i = 1;
  bool has_input = false;
  bool has_output = false;
  while (i < argc) {
    String arg = g_options.arguments[i];
    if (arg == "-ps4-pragma"){
      g_options.pragmas.push_back(g_options.arguments[++i]);
      ++i;
    } else if (arg == "-gl") {
      g_options.output_type = PROGRAM_OUTPUT_OPENGL;
      ++i;
    } else if (arg == "-dx") {
      g_options.output_type = PROGRAM_OUTPUT_DIRECTX;
      ++i;
    } else if(arg=="-ps4"){
      g_options.output_type = PROGRAM_OUTPUT_PS4;
      ++i;
    } else if (arg == "-b") {
      g_options.output_binary = true;
      ++i;
    } else if (arg.Left(2) == "-D") {
      g_options.defines.push_back(arg.Substr(2));
      ++i;
    } else if (!has_input) {
      has_input = true;
      g_options.input_filename = arg;
      ++i;
    } else if (!has_output) {
      has_output = true;
      g_options.output_filename = arg;
      ++i;
    } else break;
  }
  return true;
}

int main(int argc, char* argv[]) {
  mem_init();
  if (!ParseArguments(argc, argv)) {
    Usage();
    return -1;
  }
  if (argc < 3) return -1;
  SLParser parser;
  SLAnalyser analyser;
  if (g_options.output_type == PROGRAM_OUTPUT_OPENGL) parser.AddDefine("GL");
  else if (g_options.output_type == PROGRAM_OUTPUT_DIRECTX) parser.AddDefine("DX");
  else if (g_options.output_type == PROGRAM_OUTPUT_PS4) parser.AddDefine("PS4");
  for (auto& define : g_options.defines) parser.AddDefine(define);
  if (parser.ParseFile(g_options.input_filename)) {
    auto shader = parser.GetShader();
    if (analyser.TypeCheck(parser.GetShader())) {
      if (g_options.output_type == PROGRAM_OUTPUT_DIRECTX) {
        HLSLWriter writer;
        writer.SetOutput(g_options.output_filename);
        writer.WriteShader(shader);
      } else if (g_options.output_type == PROGRAM_OUTPUT_OPENGL) {
        GLSLWriter writer;
        writer.SetOutput(g_options.output_filename);
        writer.WriteShader(shader);
      } else {
        PSSLWriter writer;
        writer.SetOutput(g_options.output_filename);
        writer.WritePragmas( g_options.pragmas);
        writer.WriteShader(shader);
      }
    } else return -2;
  } else return -1;
  return 0;
}
