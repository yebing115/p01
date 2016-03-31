#include "C3PCH.h"
#include "EncodingConverter.h"

const int EncodingConverter::BLOCK_SIZE = 128;

EncodingConverter::EncodingConverter(Encoding from, Encoding to)
: _from(from), _to(to), _descriptor(nullptr), _block(BLOCK_SIZE) {
  _descriptor = iconv_open(ENCODING_NAMES[to], ENCODING_NAMES[from]);
}

EncodingConverter::~EncodingConverter() {
  if (_descriptor) {
    iconv_close(_descriptor);
    _descriptor = nullptr;
  }
}

String EncodingConverter::Convert(const String& text) const {
  String result;
  const char* in = text.GetCString();
  size_t in_remains = text.GetLength();

  do {
    char* out = _block.data();
    size_t out_remains = BLOCK_SIZE;
    int r = iconv(_descriptor, &in, &in_remains, &out, &out_remains);
    if (out_remains == BLOCK_SIZE) break;
    result.Append(_block.data(), BLOCK_SIZE - out_remains);
    if (r == 0) break;
  } while (true);

  return result;
}
