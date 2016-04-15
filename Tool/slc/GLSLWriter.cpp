#include "GLSLWriter.h"
#include <assert.h>

GLSLWriter::GLSLWriter(): _f(nullptr) {}
GLSLWriter::~GLSLWriter() {}

void GLSLWriter::SetOutput(const String& filename) {
  _filename = filename;
  _f = fopen(filename.GetCString(), "wb");
  assert(_f);
}

void GLSLWriter::WriteShader(ShaderNode* node) {
  _shader = node;
  WriteVersion();
  WriteNewline();
  if (!node->inputs.empty()) {
    for (auto var_decl : node->inputs) WriteInputDecl(var_decl);
    WriteNewline();
  }
  if (!node->outputs.empty()) {
    for (auto var_decl : node->outputs) WriteOutputDecl(var_decl);
    WriteNewline();
  }
  if (!node->uniforms.empty()) {
    for (auto var_decl : node->uniforms) WriteUniformDecl(var_decl);
    WriteNewline();
  }
  if (!node->var_decls.empty()) {
    for (auto var_decl : node->var_decls) WriteVarDecl(var_decl);
    WriteNewline();
  }
  if (!node->func_decls.empty()) {
    for (auto func_decl : node->func_decls) {
      WriteFuncDecl(func_decl);
      WriteNewline();
    }
  }
}

void GLSLWriter::WriteVersion() {
  fputs("#version 330\n", _f);
}

void GLSLWriter::WriteInputDecl(VarDeclNode* node) {
  WriteString("in ");
  WriteVarDecl(node);
}

void GLSLWriter::WriteOutputDecl(VarDeclNode* node) {
  WriteString("out ");
  WriteVarDecl(node);
}

void GLSLWriter::WriteUniformDecl(VarDeclNode* node) {
  WriteString("uniform ");
  WriteVarDecl(node);
}

void GLSLWriter::WriteVarDecl(VarDeclNode* node, bool write_semicolon) {
  if (write_semicolon) WriteIndent();
  WriteTypeDecl(node->type_decl);
  WriteSpace();
  WriteString(node->var_name.text);
  if (node->type_decl->num > 1) fprintf(_f, "[%d]", node->type_decl->num);
  if (node->init) {
    WriteString(" = ");
    WriteExpression(node->init);
  }
  if (write_semicolon) WriteString(";\n");
}

void GLSLWriter::WriteFuncDecl(FuncDeclNode* node) {
  WriteTypeDecl(node->ret_type);
  WriteSpace();
  WriteString(node->func_name.text);
  WriteString("(");
  for (int i = 0; i < node->num_params; ++i) {
    if (i > 0) WriteString(", ");
    if (node->params[i].flags == FUNC_PARAM_INOUT) WriteString("inout ");
    if (node->params[i].flags == FUNC_PARAM_OUT) WriteString("out ");
    WriteVarDecl(node->params[i].var_decl, false);
  }
  WriteString(") {\n");
  ++_indent;
  WriteStatements(node->statements);
  --_indent;
  WriteString("}\n");
}

void GLSLWriter::WriteVarRef(VarRefNode* node) {
  if (node->ref_type == VAR_REF_TYPE_ID) WriteString(GetOutputVarName(node->var_name->text));
  else if (node->ref_type == VAR_REF_TYPE_INDEX) {
    WriteString("[");
    WriteExpression(node->index);
    WriteString("]");
  } else if (node->ref_type == VAR_REF_TYPE_FIELD) {
    WriteString(".");
    WriteString(node->field_name->text);
  }
  if (node->suffix) WriteVarRef(node->suffix);
}

void GLSLWriter::WriteAssignStatement(AssignStatementNode* node, bool write_semicolon) {
  if (write_semicolon) WriteIndent();
  if (node->left) WriteVarRef(node->left);
  WriteOp(node->assign_type);
  WriteExpression(node->right);
  if (write_semicolon) WriteString(";\n");
}

void GLSLWriter::WriteIfStatement(IfStatementNode* node) {
  WriteIndent();
  WriteString("if (");
  WriteExpression(node->condition);
  WriteString(") {\n");
  ++_indent;
  WriteStatements(node->true_stmts);
  --_indent;
  WriteIndent();
  WriteString("}");
  if (node->false_stmts.empty()) WriteNewline();
  else {
    WriteString(" else {\n");
    ++_indent;
    WriteStatements(node->false_stmts);
    --_indent;
    WriteIndent();
    WriteString("}\n");
  }
}

void GLSLWriter::WriteForStatement(ForStatementNode* node) {
  WriteIndent();
  WriteString("for (");
  if (node->init) WriteVarDecl(node->init, false);
  WriteString(";");
  if (node->condition) {
    WriteSpace();
    WriteExpression(node->condition);
  }
  WriteString(";");
  if (node->step) {
    WriteSpace();
    WriteAssignStatement(node->step, false);
  }
  WriteString(") {\n");
  ++_indent;
  WriteStatements(node->statements);
  --_indent;
  WriteIndent();
  WriteString("}\n");
}

void GLSLWriter::WriteTypeDecl(TypeDeclNode* node, bool write_num) {
  if (node->is_const) WriteString("const ");
  WriteVarType(node->type);
  if (write_num && node->num > 1) fprintf(_f, "[%d]", node->num);
}

void GLSLWriter::WriteVarType(VarType type) {
  if (type == VAR_TYPE_BOOL) WriteString("bool");
  else if (type == VAR_TYPE_INT) WriteString("int");
  else if (type == VAR_TYPE_FLOAT) WriteString("float");
  else if (type == VAR_TYPE_VOID) WriteString("void");
  else if (type == VAR_TYPE_VEC2) WriteString("vec2");
  else if (type == VAR_TYPE_VEC3) WriteString("vec3");
  else if (type == VAR_TYPE_VEC4) WriteString("vec4");
  else if (type == VAR_TYPE_IVEC2) WriteString("ivec2");
  else if (type == VAR_TYPE_IVEC3) WriteString("ivec3");
  else if (type == VAR_TYPE_IVEC4) WriteString("ivec4");
  else if (type == VAR_TYPE_MAT2) WriteString("mat2");
  else if (type == VAR_TYPE_MAT3) WriteString("mat3");
  else if (type == VAR_TYPE_MAT4) WriteString("mat4");
  else if (type == VAR_TYPE_SAMPLER_2D) WriteString("sampler2D");
  else if (type == VAR_TYPE_SAMPLER_2D_SHADOW) WriteString("sampler2DShadow");
}

void GLSLWriter::WriteExpression(ExpressionNode* node) {
  int pred1, pred2;
  bool right_assoc1, right_assoc2;
  bool need_paren1 = false;
  bool need_paren2 = false;
  if (node->expr_type == EXPR_TYPE_NOT || node->expr_type == EXPR_TYPE_NEG ||
      node->expr_type == EXPR_TYPE_PRE_INC || node->expr_type == EXPR_TYPE_PRE_DEC) {
    WriteOp(node->expr_type);
    GetPrecedence(node->expr_type, pred1, right_assoc1);
    GetPrecedence(node->first->expr_type, pred2, right_assoc2);
    need_paren2 = pred1 > pred2 || (pred1 == pred2 && !right_assoc1);
    if (need_paren2) WriteString("(");
    WriteExpression(node->first);
    if (need_paren2) WriteString(")");
  } else if (node->expr_type == EXPR_TYPE_NUMBER) WriteString(node->number->text);
  else if (node->expr_type == EXPR_TYPE_INTEGER) WriteString(node->integer->text);
  else if (node->expr_type == EXPR_TYPE_BOOLEAN) WriteString(node->boolean ? "true" : "false");
  else if (node->expr_type == EXPR_TYPE_VAR_REF) WriteVarRef(node->var_ref);
  else if (node->expr_type == EXPR_TYPE_CAST) {
    WriteTypeDecl(node->type_cast.target_type, true);
    WriteString("(");
    for (int i = 0; i < node->type_cast.argc; ++i) {
      if (i > 0) WriteString(", ");
      WriteExpression(node->type_cast.argv[i]);
    }
    WriteString(")");
  } else if (node->expr_type == EXPR_TYPE_FUNC_CALL) {
    WriteString(node->func_call.func_name->text);
    WriteString("(");
    for (int i = 0; i < node->func_call.argc; ++i) {
      if (i > 0) WriteString(", ");
      WriteExpression(node->func_call.argv[i]);
    }
    WriteString(")");
  } else if (node->expr_type == EXPR_TYPE_SUB_FIELD) {
    GetPrecedence(node->expr_type, pred1, right_assoc1);
    GetPrecedence(node->first->expr_type, pred2, right_assoc2);
    need_paren1 = pred1 > pred2 || (pred1 == pred2 && right_assoc1);

    if (need_paren1) WriteString("(");
    WriteExpression(node->sub_field.first);
    if (need_paren1) WriteString(")");

    WriteOp(node->expr_type);
    WriteString(node->sub_field.field_name->text);
  } else if (node->expr_type == EXPR_TYPE_SUB_INDEX) {
    GetPrecedence(node->expr_type, pred1, right_assoc1);
    GetPrecedence(node->first->expr_type, pred2, right_assoc2);
    need_paren1 = pred1 > pred2 || (pred1 == pred2 && right_assoc1);

    if (need_paren1) WriteString("(");
    WriteExpression(node->first);
    if (need_paren1) WriteString(")");

    WriteString("[");
    WriteExpression(node->second);
    WriteString("]");
  } else if (node->first && node->second) {
    GetPrecedence(node->expr_type, pred1, right_assoc1);
    GetPrecedence(node->first->expr_type, pred2, right_assoc2);
    need_paren1 = pred1 > pred2 || (pred1 == pred2 && right_assoc1);
    
    if (need_paren1) WriteString("(");
    WriteExpression(node->first);
    if (need_paren1) WriteString(")");
    
    WriteOp(node->expr_type);

    GetPrecedence(node->second->expr_type, pred2, right_assoc2);
    need_paren2 = pred1 > pred2 || (pred1 == pred2 && !right_assoc1);

    if (need_paren2) WriteString("(");
    WriteExpression(node->second);
    if (need_paren2) WriteString(")");
    
  }
}

void GLSLWriter::WriteOp(ExpressionType type) {
  switch (type) {
  case EXPR_TYPE_ADD:
    WriteString(" + ");
    break;
  case EXPR_TYPE_MINUS:
    WriteString(" - ");
    break;
  case EXPR_TYPE_MUL:
  case EXPR_TYPE_VEC_MUL:
    WriteString(" * ");
    break;
  case EXPR_TYPE_DIV:
    WriteString(" / ");
    break;
  case EXPR_TYPE_NEG:
    WriteString("-");
    break;
  case EXPR_TYPE_PRE_INC:
    WriteString("++");
    break;
  case EXPR_TYPE_PRE_DEC:
    WriteString("--");
    break;
  case EXPR_TYPE_EQ:
    WriteString(" == ");
    break;
  case EXPR_TYPE_NEQ:
    WriteString(" != ");
    break;
  case EXPR_TYPE_LESS:
    WriteString(" < ");
    break;
  case EXPR_TYPE_LEQ:
    WriteString(" <= ");
    break;
  case EXPR_TYPE_GREATER:
    WriteString(" > ");
    break;
  case EXPR_TYPE_GEQ:
    WriteString(" >= ");
    break;
  case EXPR_TYPE_AND:
    WriteString(" && ");
    break;
  case EXPR_TYPE_OR:
    WriteString(" || ");
    break;
  case EXPR_TYPE_NOT:
    WriteString("!");
    break;
  case EXPR_TYPE_SUB_FIELD:
    WriteString(".");
    break;
  case EXPR_TYPE_ASSIGN:
    WriteString(" = ");
    break;
  case EXPR_TYPE_PLUS_ASSIGN:
    WriteString(" += ");
    break;
  case EXPR_TYPE_MINUS_ASSIGN:
    WriteString(" -= ");
    break;
  case EXPR_TYPE_MUL_ASSIGN:
    WriteString(" *= ");
    break;
  case EXPR_TYPE_DIV_ASSIGN:
    WriteString(" /= ");
    break;
  default:
    break;
  }
}

void GLSLWriter::WriteStatements(vector<ASTNode*> nodes) {
  for (auto stmt : nodes) {
    if (stmt->node_type == NODE_TYPE_VAR_DECL) WriteVarDecl((VarDeclNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_ASSIGN_STMT) WriteAssignStatement((AssignStatementNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_IF_STMT) WriteIfStatement((IfStatementNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_FOR_STMT) WriteForStatement((ForStatementNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_RETURN_STMT) WriteReturnStatement((ReturnStatementNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_BREAK_STMT) WriteBreakStatement((BreakStatementNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_CONTINUE_STMT) WriteContinueStatement((ContinueStatementNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_DISCARD_STMT) {
      WriteIndent();
      WriteString("discard;\n");
    }
  }
}

void GLSLWriter::WriteReturnStatement(ReturnStatementNode* node) {
  WriteIndent();
  WriteString("return");
  if (node->ret_value) {
    WriteSpace();
    WriteExpression(node->ret_value);
  }
  WriteString(";");
  WriteNewline();
}

void GLSLWriter::WriteBreakStatement(BreakStatementNode* node) {
  WriteIndent();
  WriteString("break;\n");
}

void GLSLWriter::WriteContinueStatement(ContinueStatementNode* node) {
  WriteIndent();
  WriteString("continue;\n");
}

void GLSLWriter::WriteIndent() {
  for (int i = 0; i < _indent; ++i) fputs("  ", _f);
}

void GLSLWriter::WriteSpace() {
  fputc(' ', _f);
}

void GLSLWriter::WriteNewline() {
  fputc('\n', _f);
}

void GLSLWriter::WriteString(const String& s) {
  fputs(s.GetCString(), _f);
}

const String& GLSLWriter::GetOutputVarName(const String& var_name) {
  static const String vs_pos = "gl_Position";
  static const String ps_pos = "gl_FragCoord";
  if (var_name == "POSITION") return _shader->shader_type == VERTEX_SHADER ? vs_pos : ps_pos;
  else return var_name;
}

void GLSLWriter::GetPrecedence(ExpressionType type, int& out_pred, bool& out_right_assoc) {
  out_right_assoc = false;
  switch (type) {
  case EXPR_TYPE_INTEGER:
  case EXPR_TYPE_NUMBER:
  case EXPR_TYPE_BOOLEAN:
  case EXPR_TYPE_VAR_REF:
    out_pred = 0;
    break;
  case EXPR_TYPE_SUB_INDEX:
  case EXPR_TYPE_SUB_FIELD:
    out_pred = -1;
    break;
  case EXPR_TYPE_NEG:
  case EXPR_TYPE_PRE_INC:
  case EXPR_TYPE_PRE_DEC:
  case EXPR_TYPE_CAST:
  case EXPR_TYPE_FUNC_CALL:
    out_right_assoc = true;
    out_pred = -2;
    break;
  case EXPR_TYPE_MUL:
  case EXPR_TYPE_VEC_MUL:
  case EXPR_TYPE_DIV:
    out_pred = -3;
    break;
  case EXPR_TYPE_ADD:
  case EXPR_TYPE_MINUS:
    out_pred = -4;
    break;
  case EXPR_TYPE_EQ:
  case EXPR_TYPE_NEQ:
  case EXPR_TYPE_LESS:
  case EXPR_TYPE_LEQ:
  case EXPR_TYPE_GREATER:
  case EXPR_TYPE_GEQ:
    out_pred = -5;
    break;
  case EXPR_TYPE_NOT:
    out_right_assoc = true;
    out_pred = -6;
    break;
  case EXPR_TYPE_AND:
    out_pred = -7;
    break;
  case EXPR_TYPE_OR:
    out_pred = -8;
    break;
  case EXPR_TYPE_ASSIGN:
  case EXPR_TYPE_PLUS_ASSIGN:
  case EXPR_TYPE_MINUS_ASSIGN:
  case EXPR_TYPE_MUL_ASSIGN:
  case EXPR_TYPE_DIV_ASSIGN:
    out_pred = -9;
    break;
  default:
    out_pred = -10;
  }
}
