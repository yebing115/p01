#include "C3/C3PCH.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static const size_t BUF_SIZE = 256 * 1024;

void error(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  exit(-1);
}

void usage() {
  printf("Usage:\n\tshaderc <mas_file>\n");
  exit(-1);
}

void compile_shader(JsonReader& reader, const char* out_dir, const char* src_key, const char* defines_key) {
  char cmd[1024];
  char slc_source[MAX_ASSET_NAME];
  char defines[1024] = "";
  char define_params[1024] = "";
  char out_filename[1024];
  reader.ReadString(src_key, slc_source, sizeof(slc_source));
  char* suffix = strrchr(slc_source, '.');
  if (!suffix) error("Invalid shader filename '%s'\n", slc_source);
  ++suffix;
  reader.BeginReadArray(defines_key);
  char p[MAX_MATERIAL_KEY_LEN];
  int num_defines = 0;
  while (reader.ReadStringElement(p, sizeof(p))) {
    if (strlen(p) > 0) {
      if (num_defines > 0) strcat(defines, ";");
      strcat(defines, p);
      strcat(define_params, " -D");
      strcat(define_params, p);
      ++num_defines;
    }
  }
  reader.EndReadArray();
  snprintf(out_filename, sizeof(out_filename), "%s/%s", out_dir, slc_source);
  char* s = strrchr(out_filename, '.');
  snprintf(s, out_filename + sizeof(out_filename) - s, "_%08x.%sb", String::GetID(defines), suffix);
  snprintf(cmd, sizeof(cmd), "slc -b -dx %s Shaders/Source/%s Shaders/%s", define_params, slc_source, out_filename);
  printf("Compiling %s...\n", out_filename);
  system(cmd);
}

void process(const char* fname) {
  char* buf = (char*)malloc(BUF_SIZE);
  FILE* f = fopen(fname, "rb");
  if (!f) error("Failed to open file '%s'\n", fname);
  int n = fread(buf, 1, BUF_SIZE - 1, f);
  if (n <= 0) error("Failed to read file '%s'\n", fname);
  buf[n] = 0;
  JsonReader reader(buf);
  if (!reader.IsValid()) error("Failed to parse material file '%s'\n", fname);
  char tech[64];
  char pass[64];
  char out_dir[MAX_ASSET_NAME];
  while (reader.BeginReadObject()) {
    reader.ReadString("technique", tech, sizeof(tech));
    reader.ReadString("pass", pass, sizeof(pass));
    snprintf(out_dir, sizeof(out_dir), "%s/%s", tech, pass);
    compile_shader(reader, out_dir, "vs_source", "vs_defines");
    compile_shader(reader, out_dir, "fs_source", "fs_defines");
    reader.EndReadObject();
  }
  fclose(f);
  free(buf);
  return;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    usage();
  }
  process(argv[1]);
  return 0;
}
