#pragma once

enum Encoding {
  GBK,
  UTF_8,
  UTF_16,
  ENCODING_COUNT
};

extern const char* ENCODING_NAMES[ENCODING_COUNT];
