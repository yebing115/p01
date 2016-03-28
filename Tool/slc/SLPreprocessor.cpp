#include "SLPreprocessor.h"

SLPreprocessor::SLPreprocessor(): _f(nullptr), _num_tags(0), _num_scratch(0) {
  Reset();
}

SLPreprocessor::~SLPreprocessor() {
  if (_f) fclose(_f);
}

bool SLPreprocessor::SetFilename(const String& filename) {
  _filename = filename;
  if (_f) fclose(_f);
  _f = fopen(filename.GetCString(), "rb");
  return _f != nullptr;
}

void SLPreprocessor::AddDefine(const String& define) {
  _tags[_num_tags].tag = FPPTAG_DEFINE;
  _tags[_num_tags].data = Scratch(define);
  ++_num_tags;
}

bool SLPreprocessor::Run() {
  _tags[_num_tags].tag = FPPTAG_INPUT_NAME;
  _tags[_num_tags].data = Scratch(_filename);
  _num_tags++;
  _tags[_num_tags].tag = FPPTAG_END;
  _tags[_num_tags].data = 0;
  _num_tags++;
  _result.Clear();
  return fppPreProcess(_tags) == 0;
}

String SLPreprocessor::GetResult() const { return _result; }

void SLPreprocessor::Reset() {
  memset(_scratch, 0, sizeof(_scratch));
  _num_scratch = 0;

  memset(_tags, 0, sizeof(_tags));
  _tags[0].tag = FPPTAG_BUILTINS;
  _tags[0].data = (void*)FALSE;
  _tags[1].tag = FPPTAG_INPUT;
  _tags[1].data = (void*)&SLPreprocessor::OnInput;
  _tags[2].tag = FPPTAG_OUTPUT;
  _tags[2].data = (void*)&SLPreprocessor::OnOutput;
  _tags[3].tag = FPPTAG_ERROR;
  _tags[3].data = (void*)&SLPreprocessor::OnError;
  _tags[4].tag = FPPTAG_USERDATA;
  _tags[4].data = (void*)this;
  _tags[5].tag = FPPTAG_IGNOREVERSION;
  _tags[5].data = (void*)FALSE;
  _num_tags = 6;

  AddDefine("M_E=2.71828182845904523536");
  AddDefine("M_LOG2E=1.44269504088896340736");
  AddDefine("M_LOG10E=0.434294481903251827651");
  AddDefine("M_LN2=0.693147180559945309417");
  AddDefine("M_LN10=2.30258509299404568402");
  AddDefine("M_PI=3.14159265358979323846");
  AddDefine("M_PI_2=1.57079632679489661923");
  AddDefine("M_PI_4=0.785398163397448309616");
  AddDefine("M_1_PI=0.318309886183790671538");
  AddDefine("M_2_PI=0.636619772367581343076");
  AddDefine("M_2_SQRTPI=1.12837916709551257390");
  AddDefine("M_SQRT2=1.41421356237309504880");
  AddDefine("M_SQRT1_2=0.707106781186547524401");
}

char* SLPreprocessor::OnInput(char *buffer, int size, void *userdata) {
  auto p = (SLPreprocessor*)userdata;
  return fgets(buffer, size, p->_f);
}

void SLPreprocessor::OnOutput(int c, void *userdata) {
  auto p = (SLPreprocessor*)userdata;
  p->_result.Append((char)c);
}

void SLPreprocessor::OnError(void* userdata, char* format, va_list ap) {
  vfprintf(stderr, format, ap);
}

char* SLPreprocessor::Scratch(const String& s) {
  strcpy(_scratch + _num_scratch, s.GetCString());
  auto ret = _scratch + _num_scratch;
  _num_scratch += s.GetLength() + 1;
  return ret;
}
