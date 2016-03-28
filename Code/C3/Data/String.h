#pragma once

#include "Data/DataType.h"
#include <assert.h>

typedef u32 stringid;

class String {
public:
  String();
  explicit String(int size);
  String(const char* s, int size = -1): String() { Set(s, size); }
  String(const wchar_t* s, int size = -1);
  String(const std::wstring& s): String(s.data(), s.size()) {}
  String(std::initializer_list<char> c): String(c.begin(), c.size()) {}
  inline String(const std::string& s): String(s.c_str(), s.size()) {}
  String(const String& s): String(s._buf, s._size) {}
  String(String&& s);
  ~String();
  
  /***************** encoding operations ***************/
  static String FromSystemEncoding(const String& s);
  static inline String FromSystemEncoding(const char* s, int size = -1) { return FromSystemEncoding(String(s, size)); }
  static inline String FromSystemEncoding(const std::string& s) { return FromSystemEncoding(s); }

  String GetSystemString() const;
  std::string GetString() const { return std::string(_buf, _size); }
  const char* GetCString() const { return _buf; }
  stringid GetID() const;
  static stringid GetID(const char *s);
  const char& operator [](size_t index) const { assert((int)index < _size); return _buf[index]; }
  char& operator [](size_t index) { assert((int)index < _size);  return _buf[index]; }
  inline int GetLength() const { return _size; }
  void Resize(int new_size);

  void Set(const char* s, int size = -1);
  void Set(const std::string& s) { Set(s.c_str(), s.size()); }
  void Set(const String& s);
  void Clear();
  void Reserve(int capacity);
  String& Insert(int pos, const char* s, int size = -1);
  String& Insert(int pos, const String& s) { return Insert(pos, s._buf, s._size); }
  String& Erase(int pos, int count = -1);
  String& Append(char ch);
  String& Append(const char* s, int size = -1);
  inline String& Append(const std::string& s) { return Append(s.c_str(), s.size()); }
  inline String& Append(const String& s) { return Append(s._buf, s._size); }
  String& Prepend(const char* s, int size = -1);
  inline String& Prepend(const std::string& s) { return Prepend(s.c_str(), s.size()); }
  inline String& Prepend(const String& s) { return Prepend(s._buf, s._size); }
  String Substr(int offset, int count = -1) const;
  inline String Left(int count) const { return Substr(0, count); }
  inline String Right(int count) const;
  String& StripSpaces();
  String CanonicalPath() const;

  int Compare(const String& other) const;
  int CompareI(const String& other) const;
  bool Equal(const String& other) const { return Compare(other) == 0; }
  bool EqualI(const String& other) const { return CompareI(other) == 0; }
  bool IsEmpty() const { return _size == 0; }
  bool GlobMatch(const String& pattern) const;
  bool GlobMatchI(const String& pattern) const;
  bool RegexMatch(const String& pattern) const;
  bool RegexMatchI(const String& pattern) const;
  bool StartsWith(char ch) const;
  bool StartsWith(const char* s, int size = -1) const;
  bool StartsWith(const String& s) const { return StartsWith(s._buf, s._size); }
  bool EndsWith(char ch) const;
  bool EndsWith(const char* s, int size = -1) const;
  bool EndsWith(const String& s) const { return EndsWith(s._buf, s._size); }
  int Find(char ch) const;
  int Find(const String& s) const;
  int FindLast(char ch) const;

  String& operator =(const String& other) { Set(other); return *this; }
  String& operator =(String&& other);
  String& operator +=(const String& other) { return Append(other); }

  /************************** path operations **************************************/
  String GetParent() const;
  String GetSuffixName() const;
  String& RemoveSuffix();
  String& RemoveLastSection(char sep = '/');
  String& ChangeSuffix(const String& suffix);
  String MakeWithoutSuffix() const;
  String MakeWithAnotherSuffix(const String& suffix) const;
  String& ToUpper();
  String& ToLower();
  String MakeUpper() const { return String(*this).ToUpper(); }
  String MakeLower() const { return String(*this).ToLower(); }
  void Split(vector<String>& out_sections, char sep = '/', bool ignore_empty = true) const;

private:
  void Grow(int new_size);
  void GrowNoCopy(int new_size);

  char* GetSuffixStart() const;
  const char* GetFirstSeparator(char sep = '/') const;
  static const char* GetFirstSeparator(const char* str, char sep = '/');
  char* GetLastSeparator(char sep = '/') const;

  int _capacity;
  int _size;
  char* _buf;
  mutable stringid _id;
  mutable bool _id_dirty;
};

inline bool operator ==(const String& s1, const String& s2) { return s1.Equal(s2); }
inline bool operator !=(const String& s1, const String& s2) { return !s1.Equal(s2); }
inline String operator +(const String& s1, const String& s2) { return String(s1).Append(s2); }
inline bool operator <(const String& s1, const String& s2) { return s1.Compare(s2) < 0; }
inline bool operator <=(const String& s1, const String& s2) { return s1.Compare(s2) <= 0; }
inline bool operator >(const String& s1, const String& s2) { return s1.Compare(s2) > 0; }
inline bool operator >=(const String& s1, const String& s2) { return s1.Compare(s2) >= 0; }

typedef vector<String> StringList;

namespace std {
  template<> struct hash<String> {
	inline size_t operator ()(const String& s) const {
	  return s.GetID();
	}
  };
}

extern const String EMPTY_STRING;
