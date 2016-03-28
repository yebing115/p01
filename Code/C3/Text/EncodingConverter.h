#pragma once

#include "EncodingName.h"
#include "Data/DataType.h"
#include "Data/String.h"
#include <iconv.h>

class EncodingConverter {
public:
  EncodingConverter(Encoding from, Encoding to);
  ~EncodingConverter();
  String Convert(const String& text) const;
private:
  static const int BLOCK_SIZE;
  const Encoding _from, _to;
  mutable vector<char> _block;
  iconv_t _descriptor;
};
