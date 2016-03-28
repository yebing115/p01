#pragma once
#include "EncodingConverter.h"
#include "Platform/PlatformConfig.h"
#include "Data/DataType.h"
#include "Data/String.h"

namespace EncodingUtil {
  String Convert(const String& text, Encoding from, Encoding to);
  String UTF8ToSystem(const String& text);
  String SystemToUTF8(const String& text);
};
