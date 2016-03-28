#pragma once

#include "Data/DataType.h"
#include "Data/String.h"
#include "SLParser.h"
#include <stdlib.h>
#include <stdio.h>

class GLSLWriter {
public:
  GLSLWriter();
  ~GLSLWriter();

  void SetOutput(const String& filename);
  void WriteShader(ShaderNode* node);
  void WriteVersion();
  void WriteInputDecl(VarDeclNode* node);
  void WriteOutputDecl(VarDeclNode* node);
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
  const String& GetOutputVarName(const String& var_name);
  static void GetPrecedence(ExpressionType type, int& out_pred, bool& right_assoc);

private:
  String _filename;
  FILE* _f;
  int _indent;
  ShaderNode* _shader;
};

