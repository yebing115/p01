#include "C3PCH.h"
#include "EncodingUtil.h"

struct FromTo {
  Encoding from, to;
  bool operator==(const FromTo& p) const { return from == p.from && to == p.to; }
};
namespace std {
  template <> struct hash<FromTo> {
	inline size_t operator ()(const FromTo& p) const {
      return p.from + p.to * ENCODING_COUNT;
	}
  };
}
static unordered_map<FromTo, EncodingConverter*> s_converters;

String EncodingUtil::Convert(const String& text, Encoding from, Encoding to) {
  if (from == to) return text;
  
  const EncodingConverter* converter = nullptr;
  FromTo from_to = {from, to};
  auto iter = s_converters.find(from_to);
  if (iter == s_converters.end()) {
	auto c = new EncodingConverter(from, to);
    converter = c;
    s_converters.emplace(from_to, c);
  } else {
	  converter = iter->second;
  }

  return converter->Convert(text);
}

String EncodingUtil::UTF8ToSystem(const String& text) {
#if SYSTEM_ENCODING == UTF_16
  auto s = Convert(text, UTF_8, SYSTEM_ENCODING);
  s.Append('\0');
  s.Resize(s.GetLength() - 1);
  return s;
#else
  return Convert(text, UTF_8, SYSTEM_ENCODING);
#endif
}

String EncodingUtil::SystemToUTF8(const String& text) {
  return Convert(text, SYSTEM_ENCODING, UTF_8);
}
