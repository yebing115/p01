#include "SLLexer.h"
#include <ctype.h>

const char* TOKEN_TYPE_STRING[TOKEN_COUNT] = {
  "<integer>",
  "<number>",
  "<id>",

  "{",
  "}",
  "[",
  "]",
  "(",
  ")",

  ":",
  ";",
  ",",
  "?",

  "+",
  "++",
  "-",
  "--",
  "*",
  "/",
  ".",
  "+=",
  "-=",
  "*=",
  "/=",
  "=",

  "==",
  "!=",
  "!",
  "&&",
  "||",
  "<",
  "<=",
  ">",
  ">=",

  "if",
  "elif",
  "else",
  "for",
  "break",
  "continue",
  "while",
  "return",
  "discard",

  "const",
  "uniform",
  "in",
  "out",
  "inout",
  "void",
  "bool",
  "float",
  "int",
  "ivec2",
  "ivec3",
  "ivec4",
  "vec2",
  "vec3",
  "vec4",
  "mat2",
  "mat3",
  "mat4",
  "sampler2D",
  "true",
  "false",
};

SLLexer::SLLexer(): _f(nullptr), _line(1), _column(0) {}
SLLexer::~SLLexer() {
  if (_f) fclose(_f);
}

bool SLLexer::SetFilename(const String& filename) {
  if (_f) {
    fclose(_f);
    _f = nullptr;
    _token = Token();
  }
  _filename = filename;
  _f = fopen(filename.GetCString(), "rb");
  _line = 1;
  _column = 0;
  if (_f) _token = _Next();
  return _f != nullptr;
}

void SLLexer::SetBuffer(const char* buf, int size, const String& filename) {
  auto name = tmpnam(nullptr);
  auto f = fopen(name, "wb");
  if (f) {
    _filename = filename;
    fwrite(buf, size, 1, f);
    fclose(f);
    SetFilename(name);
  }
}

Token SLLexer::_Next() {
  SkipSpaceAndComments();
  int ch = GetChar();
  if (ch == EOF) return CreateToken(TOKEN_INVALID, "EOF");
  else if (ch == '{') return CreateToken(TOKEN_LBRACE, char(ch));
  else if (ch == '}') return CreateToken(TOKEN_RBRACE, char(ch));
  else if (ch == '[') return CreateToken(TOKEN_LBRACKET, char(ch));
  else if (ch == ']') return CreateToken(TOKEN_RBRACKET, char(ch));
  else if (ch == '(') return CreateToken(TOKEN_LPAREN, char(ch));
  else if (ch == ')') return CreateToken(TOKEN_RPAREN, char(ch));
  else if (ch == ':') return CreateToken(TOKEN_COLON, char(ch));
  else if (ch == ';') return CreateToken(TOKEN_SEMICOLON, char(ch));
  else if (ch == ',') return CreateToken(TOKEN_COMMA, char(ch));
  else if (ch == '?') return CreateToken(TOKEN_QMARK, char(ch));
  else if (ch == '+') {
    if (Match('=')) return CreateToken(TOKEN_PLUS_ASSIGN, "+=");
    else if (Match('+')) return CreateToken(TOKEN_PLUS_PLUS, "++");
    else return CreateToken(TOKEN_PLUS, char(ch));
  } else if (ch == '-') {
    if (Match('=')) return CreateToken(TOKEN_MINUS_ASSIGN, "-=");
    else if (Match('-')) return CreateToken(TOKEN_MINUS_MINUS, "--");
    else if (isdigit(PeekChar()) || PeekChar() == '.') {
      String s;
      s.Reserve(8);
      s.Append(char(ch));
      int next = fgetc(_f);
      bool has_dot = false;
      while (isdigit(next) || (!has_dot && next == '.')) {
        ++_column;
        s.Append((char)next);
        if (next == '.') has_dot = true;
        next = fgetc(_f);
      }
      ungetc(next, _f);
      _column += s.GetLength();
      if (has_dot) return CreateToken(TOKEN_NUMBER, s);
      else return CreateToken(TOKEN_INTEGER, s);
    } else return CreateToken(TOKEN_MINUS, char(ch));
  } else if (ch == '*') return Match('=') ? CreateToken(TOKEN_MUL_ASSIGN, "*=") : CreateToken(TOKEN_MUL, char(ch));
  else if (ch == '/') return Match('=') ? CreateToken(TOKEN_DIV_ASSIGN, "/=") : CreateToken(TOKEN_DIV, char(ch));
  else if (ch == '=') return Match('=') ? CreateToken(TOKEN_EQ, "==") : CreateToken(TOKEN_ASSIGN, char(ch));
  else if (ch == '!') return Match('=') ? CreateToken(TOKEN_NEQ, "!=") : CreateToken(TOKEN_NOT, char(ch));
  else if (ch == '<') return Match('=') ? CreateToken(TOKEN_LEQ, "!=") : CreateToken(TOKEN_LESS, char(ch));
  else if (ch == '>') return Match('=') ? CreateToken(TOKEN_GEQ, "!=") : CreateToken(TOKEN_GREATER, char(ch));
  else if (ch == '&') return Match('&') ? CreateToken(TOKEN_AND, "&&") : CreateToken(TOKEN_INVALID, char(ch));
  else if (ch == '|') return Match('|') ? CreateToken(TOKEN_OR, "||") : CreateToken(TOKEN_INVALID, char(ch));
  else if (ch == '.') {
    if (isdigit(PeekChar())) {
      String s;
      s.Reserve(8);
      s.Append('.');
      int next = fgetc(_f);
      while (isdigit(next)) {
        ++_column;
        s.Append((char)next);
        next = fgetc(_f);
      }
      ungetc(next, _f);
      _column += s.GetLength();
      return CreateToken(TOKEN_NUMBER, s);
    } else return CreateToken(TOKEN_DOT, char(ch));
  } else {
    if (isdigit(ch)) {
      String s;
      s.Reserve(8);
      s.Append(char(ch));
      int next = fgetc(_f);
      bool has_dot = false;
      while (isdigit(next) || (!has_dot && next == '.')) {
        ++_column;
        s.Append((char)next);
        if (next == '.') has_dot = true;
        next = fgetc(_f);
      }
      ungetc(next, _f);
      _column += s.GetLength();
      if (has_dot) return CreateToken(TOKEN_NUMBER, s);
      else return CreateToken(TOKEN_INTEGER, s);
    } else if (isalpha(ch) || ch == '_') {
      String s;
      s.Reserve(16);
      s.Append(char(ch));
      int next = fgetc(_f);
      while (isalnum(next) || next == '_') {
        ++_column;
        s.Append((char)next);
        next = fgetc(_f);
      }
      ungetc(next, _f);
      
      _column += s.GetLength();
      if (s == "if") return CreateToken(TOKEN_KWORD_IF, s);
      else if (s == "elif") return CreateToken(TOKEN_KWORD_ELIF, s);
      else if (s == "else") return CreateToken(TOKEN_KWORD_ELSE, s);
      else if (s == "for") return CreateToken(TOKEN_KWORD_FOR, s);
      else if (s == "break") return CreateToken(TOKEN_KWORD_BREAK, s);
      else if (s == "continue") return CreateToken(TOKEN_KWORD_CONTINUE, s);
      else if (s == "while") return CreateToken(TOKEN_KWORD_WHILE, s);
      else if (s == "return") return CreateToken(TOKEN_KWORD_RETURN, s);
      else if (s == "discard") return CreateToken(TOKEN_KWORD_DISCARD, s);
      else if (s == "const") return CreateToken(TOKEN_KWORD_CONST, s);
      else if (s == "uniform") return CreateToken(TOKEN_KWORD_UNIFORM, s);
      else if (s == "in") return CreateToken(TOKEN_KWORD_IN, s);
      else if (s == "out") return CreateToken(TOKEN_KWORD_OUT, s);
      else if (s == "inout") return CreateToken(TOKEN_KWORD_INOUT, s);
      else if (s == "void") return CreateToken(TOKEN_KWORD_VOID, s);
      else if (s == "bool") return CreateToken(TOKEN_KWORD_BOOL, s);
      else if (s == "int") return CreateToken(TOKEN_KWORD_INT, s);
      else if (s == "float") return CreateToken(TOKEN_KWORD_FLOAT, s);
      else if (s == "ivec2") return CreateToken(TOKEN_KWORD_IVEC2, s);
      else if (s == "ivec3") return CreateToken(TOKEN_KWORD_IVEC3, s);
      else if (s == "ivec4") return CreateToken(TOKEN_KWORD_IVEC4, s);
      else if (s == "vec2") return CreateToken(TOKEN_KWORD_VEC2, s);
      else if (s == "vec3") return CreateToken(TOKEN_KWORD_VEC3, s);
      else if (s == "vec4") return CreateToken(TOKEN_KWORD_VEC4, s);
      else if (s == "mat2") return CreateToken(TOKEN_KWORD_MAT2, s);
      else if (s == "mat3") return CreateToken(TOKEN_KWORD_MAT3, s);
      else if (s == "mat4") return CreateToken(TOKEN_KWORD_MAT4, s);
      else if (s == "sampler2D") return CreateToken(TOKEN_KWORD_SAMPLER_2D, s);
      else if (s == "true") return CreateToken(TOKEN_KWORD_TRUE, s);
      else if (s == "false") return CreateToken(TOKEN_KWORD_FALSE, s);
      else return CreateToken(TOKEN_ID, s);
    } else return CreateToken(TOKEN_INVALID, String{(char)ch});
  } 
}

int SLLexer::Save() {
  auto offset = ftell(_f);
  _saved_tokens[offset] = _token;
  return offset;
}

void SLLexer::Restore(int offset) {
  auto it = _saved_tokens.find(offset);
  assert(it != _saved_tokens.end());
  fseek(_f, offset, SEEK_SET);
  _token = it->second;
}

const char* SLLexer::GetTokenTypeName(TokenType type) {
  if (type == TOKEN_INVALID) return "<invalid>";
  return TOKEN_TYPE_STRING[type];
}

Token SLLexer::Next() {
  Token t = _token;
  _token = _Next();
  return t;
}

Token SLLexer::CreateToken(TokenType t, char ch) {
  FileLocation loc(_filename, _line, _column - 1);
  return Token(t, ch, loc);
}

Token SLLexer::CreateToken(TokenType t, const String& s) {
  FileLocation loc(_filename, _line, _column - s.GetLength());
  return Token(t, s, loc);
}

bool SLLexer::Match(char ch) {
  int c = fgetc(_f);
  if (c == ch) {
    ++_column;
    return true;
  } else {
    ungetc(c, _f);
    //fprintf(stderr, "%s(%d,%d): Expect '%c', got '%c'\n", _filename.GetCString(), _line, _column, ch, c);
    return false;
  }
}

bool SLLexer::Match(TokenType type, Token* token) {
  if (_token.type == type) {
    if (token) *token = _token;
    _token = _Next();
    return true;
  } else {
    fprintf(stderr, "%s(%d,%d): Expect '%s', got '%s'\n", _token.location.filename.GetCString(),
            _token.location.line, _token.location.column, GetTokenTypeName(type), _token.text.GetCString());
    return false;
  }
}

void SLLexer::SkipSpaceAndComments() {
  int ch;
  while (true) {
    ch = fgetc(_f);
    if (ch != EOF && isspace(ch)) {
      if (ch == '\n') MarkNewline();
      else ++_column;
    } else if (ch == '#') {
      String pragma;
      while (ch != EOF && ch != '\n') {
        pragma.Append((char)ch);
        ch = fgetc(_f);
      }
      if (pragma.Left(5) == "#line") LinePragma(pragma);
      else if (ch == '\n') MarkNewline();
    } else break;
  }
  ungetc(ch, _f);
}

int SLLexer::PeekChar() const {
  int ch = fgetc(_f);
  ungetc(ch, _f);
  return ch;
}

int SLLexer::GetChar() {
  int ch = fgetc(_f);
  if (ch == '\n') MarkNewline();
  else if (ch != EOF) ++_column;
  return ch;
}

void SLLexer::MarkNewline() {
  _line_locations.resize(_line);
  _line_locations[_line - 1] = FileLocation(_filename, _line, _column);
  ++_line;
  _column = 1;
}

String SLLexer::PeekString(int n) {
  int offset = Save();
  String s;
  for (int i = 0; i < n; ++i) {
    int ch = fgetc(_f);
    if (ch == EOF) break;
    s.Append(char(ch));
  }
  Restore(offset);
  return s;
}

void SLLexer::LinePragma(const String& pragma) {
  int line;
  char filename[1024];
  int n = sscanf(pragma.GetCString(), "#line %d \"%[^\"]\"", &line, filename);
  if (n >= 1) {
    _line = line;
    _column = 1;
  } 
  if (n == 2) _filename = filename;
}
