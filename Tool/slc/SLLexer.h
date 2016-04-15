#pragma once

#include "Data/DataType.h"
#include "Data/String.h"
#include <stdio.h>
#include <stdlib.h>

enum TokenType {
  TOKEN_INVALID = -1,

  TOKEN_INTEGER,
  TOKEN_NUMBER,
  TOKEN_ID,

  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_LPAREN,
  TOKEN_RPAREN,

  TOKEN_COLON,
  TOKEN_SEMICOLON,
  TOKEN_COMMA,
  TOKEN_QMARK,

  TOKEN_PLUS,
  TOKEN_PLUS_PLUS,
  TOKEN_MINUS,
  TOKEN_MINUS_MINUS,
  TOKEN_MUL,
  TOKEN_DIV,
  TOKEN_DOT,
  TOKEN_PLUS_ASSIGN,
  TOKEN_MINUS_ASSIGN,
  TOKEN_MUL_ASSIGN,
  TOKEN_DIV_ASSIGN,
  TOKEN_ASSIGN,

  TOKEN_EQ,
  TOKEN_NEQ,
  TOKEN_NOT,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_LESS,
  TOKEN_LEQ,
  TOKEN_GREATER,
  TOKEN_GEQ,

  TOKEN_KWORD_IF,
  TOKEN_KWORD_ELIF,
  TOKEN_KWORD_ELSE,
  TOKEN_KWORD_FOR,
  TOKEN_KWORD_BREAK,
  TOKEN_KWORD_CONTINUE,
  TOKEN_KWORD_WHILE,
  TOKEN_KWORD_RETURN,
  TOKEN_KWORD_DISCARD,

  TOKEN_KWORD_CONST,
  TOKEN_KWORD_UNIFORM,
  TOKEN_KWORD_IN,
  TOKEN_KWORD_OUT,
  TOKEN_KWORD_INOUT,
  TOKEN_KWORD_VOID,
  TOKEN_KWORD_BOOL,
  TOKEN_KWORD_FLOAT,
  TOKEN_KWORD_INT,
  TOKEN_KWORD_IVEC2,
  TOKEN_KWORD_IVEC3,
  TOKEN_KWORD_IVEC4,
  TOKEN_KWORD_VEC2,
  TOKEN_KWORD_VEC3,
  TOKEN_KWORD_VEC4,
  TOKEN_KWORD_MAT2,
  TOKEN_KWORD_MAT3,
  TOKEN_KWORD_MAT4,
  TOKEN_KWORD_SAMPLER_2D,
  TOKEN_KWORD_SAMPLER_2D_SHADOW,

  TOKEN_KWORD_TRUE,
  TOKEN_KWORD_FALSE,

  TOKEN_COUNT,
};

struct FileLocation {
  String filename;
  int line;
  int column;
  
  FileLocation(): line(0), column(0) {}
  FileLocation(const String& name, int l, int c): filename(name), line(l), column(c) {}
  String ToString() const {
    char buf[256];
    sprintf(buf, "%s(%d,%d)", filename.GetCString(), line, column);
    return String(buf);
  }
};

extern const char* TOKEN_TYPE_STRING[TOKEN_COUNT];

struct Token {
  TokenType type;
  String text;
  FileLocation location;

  Token(): type(TOKEN_INVALID) {}
  Token(TokenType t, char ch, const FileLocation& loc): type(t), text(&ch, 1), location(loc) {}
  Token(TokenType t, const String& text_, const FileLocation& loc): type(t), text(text_), location(loc) {}
  float ToNumber() const { return strtof(text.GetCString(), nullptr); }
  int ToInt() const { return atoi(text.GetCString()); }
  char ToChar() const { return text[0]; }
  bool IsDataType() const { return type >= TOKEN_KWORD_VOID && type <= TOKEN_KWORD_SAMPLER_2D_SHADOW; }

  operator bool() const { return type != TOKEN_INVALID; }
};

class SLLexer {
public:
  SLLexer();
  ~SLLexer();

  bool SetFilename(const String& filename);
  void SetBuffer(const char* buf, int size, const String& filename);

  Token Peek() const { return _token; }
  Token Next();
  bool Match(TokenType type, Token* token);
    
  int Save();
  void Restore(int offset);

  static const char* GetTokenTypeName(TokenType type);

private:
  Token _Next();
  Token CreateToken(TokenType t, char ch);
  Token CreateToken(TokenType t, const String& s);
  bool Match(char ch);
  void SkipSpaceAndComments();
  int PeekChar() const;
  int GetChar();
  void UngetChar();
  void MarkNewline();
  String PeekString(int n);
  void LinePragma(const String& line);
  String _filename;
  int _line;
  int _column;
  vector<FileLocation> _line_locations;
  FILE* _f;
  Token _token;
  unordered_map<int, Token> _saved_tokens;
};
