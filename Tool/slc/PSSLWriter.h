#pragma once

#include "Data/DataType.h"
#include "Data/String.h"
#include "SLParser.h"
#include <stdlib.h>
#include <stdio.h>

class PSSLWriter {
public:
  PSSLWriter();
  ~PSSLWriter();

  void SetOutput(const String& filename);
  void WritePragmas(vector<String>& pragmas);
  void WriteShader(ShaderNode* node);
  void WriteVersion();
  void WriteInputDecl(VarDeclNode* node);
  void WriteOutputDecl(VarDeclNode* node, int index);
  void WriteUniformDecl(VarDeclNode* node);
  void WriteVarDecl(VarDeclNode* node, bool write_semicolon = true);
  void WriteFuncDecl(FuncDeclNode* node);
  void WriteVarRef(VarRefNode* node);
  void WriteAssignStatement(AssignStatementNode* node, bool write_semicolon = true);
  void WriteIfStatement(IfStatementNode* node);
  void WriteForStatement(ForStatementNode* node);
  void WriteTypeDecl(TypeDeclNode* node, bool write_num = false);
  void WriteVarType(VarType type);
  void WriteExpression(ExpressionNode* node);
  void WriteOp(ExpressionType type);
  void WriteStatements(vector<ASTNode*> nodes);
  void WriteReturnStatement(ReturnStatementNode* node);
  void WriteBreakStatement(BreakStatementNode* node);
  void WriteContinueStatement(ContinueStatementNode* node);

  void WriteIndent();
  void WriteSpace();
  void WriteNewline();
  void WriteString(const String& s);
  String GetVarName(const String& var_name);
  String GetFuncName(const String& func_name, int arg_count);
  static void GetPrecedence(ExpressionType type, int& out_pred, bool& right_assoc);

private:
  static bool IsSamplerFuncName(const String& func_name);
  const String& GetInputStructName() const;
  const String& GetOutputStructName() const;
  void WriteBuiltinFunctions();
  String _filename;
  FILE* _f;
  int _indent;
  ShaderNode* _shader;
  bool is_in_function;
};

