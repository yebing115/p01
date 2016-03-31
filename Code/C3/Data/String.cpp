#include "C3PCH.h"
#include "Algorithm/Hasher.h"
#include "Text/EncodingUtil.h"
#include "String.h"
#include "Platform/PlatformConfig.h"
#if ON_WINDOWS
#include "Platform/Windows/WindowsHeader.h"
#endif
#include <string.h>
#include <regex>

#define ROUND_UP(x) ((x + 15) & ~15)

static char* s_empty_string = (char*)"";
const String EMPTY_STRING;

static bool glob_match(const char* str, const char* pat, bool (*char_eq)(char, char)) {
  const char* s = str;
  const char* p = pat;
  bool star = false;

loop_start:
  for (s = str, p = pat; *s; ++s, ++p) {
    switch (*p) {
    case '?':
      if (*s == '.') goto star_check;
      break;
    case '*':
      star = true;
      str = s, pat = p;
      if (!*++pat) return true;
      goto loop_start;
    default:
      if (!char_eq(*s, *p)) goto star_check;
      break;
    }
  }
  if (*p == '*') ++p;
  return (!*p);

star_check:
  if (!star) return false;
  str++;
  goto loop_start;

}

String::String(): _capacity(0), _size(0), _buf(s_empty_string), _id(0), _id_dirty(false) {}

String::String(int size) : _capacity(ROUND_UP(size + 1)), _size(size), _id(0), _id_dirty(false) {
  _buf = (char*)malloc(_capacity);
  if (_buf) _buf[size] = 0;
}

String::String(String&& s) : _capacity(s._capacity), _size(s._size), _buf(s._buf), _id(s._id), _id_dirty(s._id_dirty) {
  s._capacity = 0;
  s._buf = s_empty_string;
  s._id = 0;
  s._id_dirty = false;
}

String::String(const wchar_t* s, int size): String() {
#if ON_WINDOWS
  if (size == -1) size = wcslen(s);
  int utf8_size = ::WideCharToMultiByte(CP_UTF8, 0, s, size, NULL, 0, NULL, NULL);
  if (utf8_size <= 0) return;
  Resize(utf8_size);
  ::WideCharToMultiByte(CP_UTF8, 0, s, size, _buf, utf8_size, NULL, NULL);

#else
#error "implement unicode to utf-8 convert"
#endif
}

String::~String() {
  if (_buf != s_empty_string) free(_buf);
}

String String::FromSystemEncoding(const String& s) {
  return EncodingUtil::SystemToUTF8(s);
}

String String::GetSystemString() const {
  return EncodingUtil::UTF8ToSystem(*this);
}

stringid String::GetID() const {
  if (_id_dirty) {
    _id = hash_string(_buf);
    _id_dirty = false;
  }
  return _id;
}

stringid String::GetID(const char *s) {
  return hash_string(s);
}

void String::Resize(int new_size) {
  if (_size != new_size) {
    Grow(new_size + 1);
    _buf[new_size] = 0;
    _size = new_size;
    _id_dirty = true;
  }
}

void String::Set(const char* s, int size) {
  if (size == -1) size = strlen(s);
  if (size == 0) {
    if (_size > 0) _buf[0] = 0;
    _size = 0;
    return;
  }
  GrowNoCopy(ROUND_UP(size + 1));
  memcpy(_buf, s, size);
  _buf[size] = 0;
  _id_dirty = true;
  _size = size;
}

void String::Set(const String& s) {
  Set(s._buf, s._size);
  _id = s._id;
  _id_dirty = s._id_dirty;
}

void String::Clear() {
  if (_size > 0) {
    _size = 0;
    _buf[0] = 0;
  }
}

void String::Reserve(int capacity) {
  Grow(ROUND_UP(capacity));
}

String& String::Insert(int pos, const char* s, int size) {
  if (size == -1) size = strlen(s);
  if (size > 0) {
    Grow(_size + size + 1);
    memmove(_buf + pos + size, _buf + pos, _size - pos);
    memcpy(_buf + pos, s, size);
    _size += size;
  }
  return *this;
}

String& String::Erase(int pos, int count) {
  if (count == -1) count = _size - pos;
  count = min(count, _size - pos);
  if (count > 0) {
    memmove(_buf + pos, _buf + pos + count, _size - (pos + count));
    _size -= count;
    _buf[_size] = 0;
  }
  return *this;
}

String& String::Append(char ch) {
  assert(_buf);

  Grow(_size + 2);
  _buf[_size] = ch;
  ++_size;
  _buf[_size] = 0;
  _id_dirty = true;
  return *this;
}

String& String::Append(const char* s, int size) {
  assert(_buf && s);

  if (size == -1) size = strlen(s);
  Grow(_size + size + 1);
  memcpy(_buf + _size, s, size);
  _size += size;
  _buf[_size] = 0;
  _id_dirty = true;
  return *this;
}

String& String::Prepend(const char* s, int size) {
  assert(_buf && s);

  if (size == -1) size = strlen(s);
  Grow(_size + size + 1);
  memmove(_buf + size, _buf, _size + 1);
  _size += size;
  memcpy(_buf, s, size);
  _id_dirty = true;
  return *this;
}

String String::Substr(int offset, int count) const {
  if (offset >= _size) return String();
  if (count == -1) count = _size - offset;
  return String(_buf + offset, count);
}

String String::Right(int count) const {
  if (count <= 0) return String();
  else if (count >= _size) return *this;
  else return Substr(_size - count, count);
}

String& String::StripSpaces() {
  const char* p = _buf + _size - 1;
  for (; p >= _buf && isspace((unsigned char)*p); p--);
  Resize(p + 1 - _buf);
  return *this;
}

String String::CanonicalPath() const {
  vector<String> sections;
  Split(sections);
  size_t i = 0;
  while (i < sections.size()) {
    if (sections[i] == ".") sections.erase(sections.begin() + i);
    else if (sections[i] == "..") {
      if (i > 0) {
        sections.erase(sections.begin() + (i - 1), sections.begin() + (i + 1));
        --i;
      } else sections.erase(sections.begin() + i);
    } else ++i;
  }
  String result;
  for (i = 0; i < sections.size() - 1; ++i) {
    result += sections[i];
    result += "/";
  }
  if (!sections.empty()) result += sections.back();
  return result;
}

int String::Compare(const String& other) const {
  return strcmp(_buf, other._buf);
}

int String::CompareI(const String& other) const {
  return _stricmp(_buf, other._buf);
}

bool String::GlobMatch(const String& pattern) const {
  return glob_match(_buf, pattern._buf, [](char c1, char c2) { return c1 == c2; });
}

bool String::GlobMatchI(const String& pattern) const {
  return glob_match(_buf, pattern._buf, [](char c1, char c2) { return toupper(c1) == toupper(c2); });
}

bool String::RegexMatch(const String& pattern) const {
  return std::regex_match(_buf, std::regex(pattern._buf, std::regex_constants::basic));
}

bool String::RegexMatchI(const String& pattern) const {
  return std::regex_match(_buf, std::regex(pattern._buf, std::regex_constants::basic | std::regex_constants::icase));
}

bool String::StartsWith(const char* s, int size) const {
  if (size == -1) size = strlen(s);
  if (_size < size) return false;
  return memcmp(_buf, s, size) == 0;
}

bool String::EndsWith(char ch) const {
  return _size > 0 && _buf[_size - 1] == ch;
}

bool String::EndsWith(const char* s, int size) const {
  if (size == -1) size = strlen(s);
  if (_size < size) return false;
  return memcmp(_buf + _size - size, s, size) == 0;
}

int String::Find(char ch) const {
  auto p = (char*)memchr(_buf, ch, _size);
  if (p) return p - _buf;
  else return -1;
}

int String::Find(const String& s) const {
  auto p = strstr(_buf, s._buf);
  if (p) return p - _buf;
  else return -1;
}

int String::FindLast(char ch) const {
  auto p = _buf + _size;
  while (--p >= _buf) {
    if (*p == ch) return p - _buf;
  }
  return -1;
}

String& String::operator=(String&& other) {
  Set(other._buf, other._size);
  _id = other._id;
  _id_dirty = other._id_dirty;
  other._capacity = 0;
  other._size = 0;
  other._buf = s_empty_string;
  other._id = 0;
  other._id_dirty = false;
  return *this;
}

String String::GetParent() const {
  char* p = GetLastSeparator();
  if (!p) return String();
  else return String(_buf, (p + 1) - _buf);
}

String String::GetSuffixName() const {
  auto p = GetSuffixStart();
  if (p) return String(p, _size - (p - _buf));
  else return String();
}

String& String::RemoveSuffix() {
  auto p = GetSuffixStart();
  if (p) {
    *p = 0;
    _size = p - _buf;
    _id_dirty = true;
  }
  return *this;
}

String& String::RemoveLastSection(char sep) {
  char* p = GetLastSeparator(sep);
  if (p) {
    *p = 0;
    _size = p - _buf;
    _id_dirty = true;
  }
  return *this;
}

String String::GetLastSection(char sep) {
  auto p = GetLastSeparator(sep);
  if (!p) return EMPTY_STRING;
  return String(p + 1);
}

String& String::ChangeSuffix(const String& suffix) {
  return RemoveSuffix().Append(suffix);
}

String String::MakeWithoutSuffix() const {
  auto p = GetSuffixStart();
  if (!p) return *this;
  else return String(_buf, p - _buf);
}

String String::MakeWithAnotherSuffix(const String& suffix) const {
  auto p = GetSuffixStart();
  if (!p) return *this + suffix;
  else return String(_buf, p - _buf) + suffix;
}

String& String::ToUpper() {
  char* p = _buf;
  while (p < _buf + _size) {
    *p = (char)toupper(*p);
    ++p;
  }
  return *this;
}

String& String::ToLower() {
  char* p = _buf;
  while (p < _buf + _size) {
    *p = (char)tolower(*p);
    ++p;
  }
  return *this;
}

void String::Split(vector<String>& out_sections, char sep, bool ignore_empty) const {
  out_sections.resize(0);
  const char *s = _buf;
  while (s < _buf + _size) {
    const char* p = GetFirstSeparator(s, sep);
    if (!p) {
      out_sections.push_back(s);
      break;
    } else if (p - s > 0) {
      out_sections.push_back({s, p - s});
      s = p + 1;
    } else if (!ignore_empty) {
      out_sections.push_back({});
      s = p + 1;
    }
  }
}

void String::Grow(int new_capacity) {
  assert(_buf);
  if (_capacity >= new_capacity) return;
  char* new_buf = (char*)malloc(new_capacity);
  memcpy(new_buf, _buf, _size + 1);
  if (_buf != s_empty_string) free(_buf);
  _buf = new_buf;
  _capacity = new_capacity;
}

void String::GrowNoCopy(int new_capacity) {
  if (_capacity >= new_capacity) return;
  char* new_buf = (char*)malloc(new_capacity);
  if (_buf != s_empty_string) free(_buf);
  _buf = new_buf;
  _capacity = new_capacity;
}

char* String::GetSuffixStart() const {
  char* s = _buf + _size;
  while (s-- >= _buf) {
    if (*s == '.') return s;
    else if (*s == '/') return nullptr;
  }
  return nullptr;
}

const char* String::GetFirstSeparator(char sep) const {
  return GetFirstSeparator(_buf, sep);
}

const char* String::GetFirstSeparator(const char* str, char sep) {
  const char* s = str;
  while (*s) {
    if (*s == sep) return s;
    ++s;
  }
  return nullptr;
}

char* String::GetLastSeparator(char sep) const {
  char* s = _buf + _size - 1;
  while (s-- >= _buf) {
    if (*s == sep) return s;
  }
  return nullptr;
}
