#include "SLParser.h"
#include <stdarg.h>

const char* VAR_TYPE_NAMES[VAR_TYPE_COUNT] = {
  "void",
  "int",
  "float",
  "bool",
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
  "sampler2DShadow",
};

#define SL_CHECK(x) \
  do { if (!(x)) { Error("%s(%d): %s", __FUNCTION__, __LINE__, #x); return false; } } while (0)

SLParser::SLParser() {}
SLParser::~SLParser() {}

bool SLParser::ParseFile(const String& filename) {
  if (!_prep.SetFilename(filename)) {
    Error("Failed to open file '%s'.", filename.GetCString());
    return false;
  }
  if (!_prep.Run()) {
    Error("Failed to preprocess file '%s'.", filename.GetCString());
    return false;
  }
  auto prep_result = _prep.GetResult();
  _lexer.SetBuffer(prep_result.GetCString(), prep_result.GetLength(), filename);
  _shader = nullptr;
  ShaderType shader_type = INVALID_SHADER;
  auto suffix = filename.GetSuffixName().ToLower();
  if (suffix == ".vs" || suffix == ".vsh" || suffix == ".vert") shader_type = VERTEX_SHADER;
  else if (suffix == ".fs" || suffix == ".fsh" || suffix == ".frag") shader_type = FRAGMENT_SHADER;
  if (shader_type == INVALID_SHADER) {
    Error("Unknown shader type '%s'.", filename.GetCString());
    return false;
  }
  return Shader(_shader, shader_type);
}

bool SLParser::Shader(ShaderNode*& node, ShaderType shader_type) {
  AllocNode(node, NODE_TYPE_SHADER);
  node->shader_type = shader_type;
  auto token = _lexer.Peek();
  while (token) {
    union {
      ASTNode* ast_node;
      VarDeclNode* var_decl;
      FuncDeclNode* func_decl;
      AssignStatementNode* assign_stmt;
      DiscardStatementNode* discard_stmt;
      ReturnStatementNode* return_stmt;
      IfStatementNode* if_stmt;
    };
    ast_node = nullptr;
    if (token.type == TOKEN_KWORD_IN) {
      SL_CHECK(InputDecl(var_decl));
      node->inputs.push_back(var_decl);
    } else if (token.type == TOKEN_KWORD_OUT) {
      SL_CHECK(OutputDecl(var_decl));
      node->outputs.push_back(var_decl);
    } else if (token.type == TOKEN_KWORD_UNIFORM) {
      UniformDecl(var_decl);
      node->uniforms.push_back(var_decl);
    } else {
      SL_CHECK(VarOrFuncDecl(ast_node));
      if (ast_node->node_type == NODE_TYPE_FUNC_DECL) node->func_decls.push_back(func_decl);
      else node->var_decls.push_back(var_decl);
    }
    token = _lexer.Peek();
  }
  return true;
}

bool SLParser::InputDecl(VarDeclNode*& node) {
  SL_CHECK(Match(TOKEN_KWORD_IN));
  SL_CHECK(VarDecl(node));
  return true;
}

bool SLParser::OutputDecl(VarDeclNode*& node) {
  SL_CHECK(Match(TOKEN_KWORD_OUT));
  SL_CHECK(VarDecl(node));
  return true;
}

bool SLParser::UniformDecl(VarDeclNode*& node) {
  SL_CHECK(Match(TOKEN_KWORD_UNIFORM));
  SL_CHECK(VarDecl(node));
  return true;
}

bool SLParser::VarOrFuncDecl(ASTNode*& node) {
  union {
    VarDeclNode* var_decl;
    FuncDeclNode* func_decl;
  };
  int pos = _lexer.Save();
  SL_CHECK(VarDecl(var_decl, false));
  if (_lexer.Peek().type == TOKEN_SEMICOLON) {
    _lexer.Next();
    node = var_decl;
  } else {
    _lexer.Restore(pos);
    SL_CHECK(FuncDecl(func_decl));
    node = func_decl;
  }
  return true;
}

// TypeDecl TOKEN_ID TOKEN_SEMICOLON
bool SLParser::VarDecl(VarDeclNode*& node, bool match_semicolon) {
  TypeDeclNode* type_decl = nullptr;
  Token id;
  SL_CHECK(TypeDecl(type_decl));
  SL_CHECK(Match(TOKEN_ID, &id));
  AllocNode(node, NODE_TYPE_VAR_DECL);
  node->type_decl = type_decl;
  node->var_name = id;
  node->node_type = NODE_TYPE_VAR_DECL;
  node->init = nullptr;
  if (_lexer.Peek().type == TOKEN_ASSIGN) {
    _lexer.Next();
    SL_CHECK(Expression(node->init));
  }
  if (match_semicolon) SL_CHECK(Match(TOKEN_SEMICOLON));
  return true;
}

bool SLParser::FuncDecl(FuncDeclNode*& node) {
  AllocNode(node, NODE_TYPE_FUNC_DECL);
  SL_CHECK(TypeDecl(node->ret_type));
  SL_CHECK(Match(TOKEN_ID, &node->func_name));
  SL_CHECK(Match(TOKEN_LPAREN));
  memset(node->params, 0, sizeof(node->params));
  node->num_params = 0;
  if (_lexer.Peek().type != TOKEN_RPAREN) {
    FetchFuncParamFlag(&node->params[node->num_params].flags);
    SL_CHECK(VarDecl(node->params[node->num_params].var_decl, false));
    node->num_params++;
    while (_lexer.Peek().type != TOKEN_RPAREN) {
      SL_CHECK(Match(TOKEN_COMMA));
      FetchFuncParamFlag(&node->params[node->num_params].flags);
      SL_CHECK(VarDecl(node->params[node->num_params].var_decl, false));
      node->num_params++;
      SL_CHECK(node->num_params < 8);
    }
  }
  SL_CHECK(Match(TOKEN_RPAREN));
  SL_CHECK(_lexer.Peek().type == TOKEN_LBRACE);
  SL_CHECK(StatementBlock(node->statements));
  return true;
}

bool SLParser::AssignStatement(AssignStatementNode*& node, bool match_semicolon) {
  AllocNode(node, NODE_TYPE_ASSIGN_STMT);
  auto token = _lexer.Peek();
  if (token.type == TOKEN_PLUS_PLUS) {
    node->assign_type = EXPR_TYPE_PRE_INC;
    node->left = nullptr;
    _lexer.Next();
    SL_CHECK(ArithExpression(node->right));
    return true;
  } else if (token.type == TOKEN_MINUS_MINUS) {
    node->assign_type = EXPR_TYPE_PRE_DEC;
    node->left = nullptr;
    _lexer.Next();
    SL_CHECK(ArithExpression(node->right));
    return true;
  }
  SL_CHECK(VarRef(node->left));
  token = _lexer.Peek();
  if (token.type == TOKEN_ASSIGN) node->assign_type = EXPR_TYPE_ASSIGN;
  else if (token.type == TOKEN_PLUS_ASSIGN) node->assign_type = EXPR_TYPE_PLUS_ASSIGN;
  else if (token.type == TOKEN_MINUS_ASSIGN) node->assign_type = EXPR_TYPE_MINUS_ASSIGN;
  else if (token.type == TOKEN_MUL_ASSIGN) node->assign_type = EXPR_TYPE_MUL_ASSIGN;
  else if (token.type == TOKEN_DIV_ASSIGN) node->assign_type = EXPR_TYPE_DIV_ASSIGN;
  else {
    Error("%s: Unknown assignment '%s'.", token.location.ToString().GetCString(), token.text.GetCString());
    return false;
  }
  _lexer.Next();
  SL_CHECK(ArithExpression(node->right));
  if (match_semicolon) SL_CHECK(Match(TOKEN_SEMICOLON));
  return true;
}

bool SLParser::DiscardStatement(DiscardStatementNode*& node) {
  AllocNode(node, NODE_TYPE_DISCARD_STMT);
  SL_CHECK(Match(TOKEN_KWORD_DISCARD));
  SL_CHECK(Match(TOKEN_SEMICOLON));
  return true;
}

bool SLParser::ContinueStatement(ContinueStatementNode*& node) {
  AllocNode(node, NODE_TYPE_CONTINUE_STMT);
  SL_CHECK(Match(TOKEN_KWORD_CONTINUE));
  SL_CHECK(Match(TOKEN_SEMICOLON));
  return true;
}

bool SLParser::BreakStatement(BreakStatementNode*& node) {
  AllocNode(node, NODE_TYPE_BREAK_STMT);
  SL_CHECK(Match(TOKEN_KWORD_BREAK));
  SL_CHECK(Match(TOKEN_SEMICOLON));
  return true;
}

bool SLParser::ReturnStatement(ReturnStatementNode*& node) {
  AllocNode(node, NODE_TYPE_RETURN_STMT);
  node->ret_value = nullptr;
  SL_CHECK(Match(TOKEN_KWORD_RETURN));
  if (_lexer.Peek().type != TOKEN_SEMICOLON) SL_CHECK(Expression(node->ret_value));
  SL_CHECK(Match(TOKEN_SEMICOLON));
  return true;
}

bool SLParser::IfStatement(IfStatementNode*& node) {
  return ElifStatement(node);
}

bool SLParser::ElifStatement(IfStatementNode*& node) {
  AllocNode(node, NODE_TYPE_IF_STMT);
  auto peek_token = _lexer.Peek();
  if (peek_token.type == TOKEN_KWORD_IF || peek_token.type == TOKEN_KWORD_ELIF) _lexer.Next();
  else {
    Error("Expect 'if' or 'elif', got '%s'", peek_token.text.GetCString());
    return false;
  }
  SL_CHECK(Match(TOKEN_LPAREN));
  SL_CHECK(Expression(node->condition));
  SL_CHECK(Match(TOKEN_RPAREN));
  SL_CHECK(StatementBlock(node->true_stmts));
  if (_lexer.Peek().type == TOKEN_KWORD_ELSE) {
    SL_CHECK(Match(TOKEN_KWORD_ELSE));
    SL_CHECK(StatementBlock(node->false_stmts));
  } else if (_lexer.Peek().type == TOKEN_KWORD_ELIF) {
    IfStatementNode* elif_stmt = nullptr;
    SL_CHECK(ElifStatement(elif_stmt));
    node->false_stmts.push_back(elif_stmt);
  }
  return true;
}

bool SLParser::ForStatement(ForStatementNode*& node) {
  AllocNode(node, NODE_TYPE_FOR_STMT);
  SL_CHECK(Match(TOKEN_KWORD_FOR));
  SL_CHECK(Match(TOKEN_LPAREN));
  node->init = nullptr;
  if (_lexer.Peek().type != TOKEN_SEMICOLON) SL_CHECK(VarDecl(node->init));
  else _lexer.Next();
  node->condition = nullptr;
  if (_lexer.Peek().type != TOKEN_SEMICOLON) SL_CHECK(RelationExpression(node->condition));
  _lexer.Next();
  node->step = nullptr;
  if (_lexer.Peek().type != TOKEN_RPAREN) SL_CHECK(AssignStatement(node->step, false));
  SL_CHECK(Match(TOKEN_RPAREN));
  SL_CHECK(StatementBlock(node->statements));
  return true;
}

bool SLParser::TypeDecl(TypeDeclNode*& node) {
  bool is_const = false;
  auto token = _lexer.Peek();
  if (token.type == TOKEN_KWORD_CONST) {
    is_const = true;
    _lexer.Next();
    token = _lexer.Peek();
  }
  if (!token.IsDataType()) {
    Error("%s: Expect type decl.", token.location.ToString().GetCString());
    return false;
  }
  auto type_token = token;
  _lexer.Next();

  AllocNode(node, NODE_TYPE_TYPE_DECL);
  node->is_const = is_const;
  if (token.type == TOKEN_KWORD_BOOL) node->type = VAR_TYPE_BOOL;
  else if (token.type == TOKEN_KWORD_INT) node->type = VAR_TYPE_INT;
  else if (token.type == TOKEN_KWORD_FLOAT) node->type = VAR_TYPE_FLOAT;
  else if (token.type == TOKEN_KWORD_VOID) node->type = VAR_TYPE_VOID;
  else if (token.type == TOKEN_KWORD_IVEC2) node->type = VAR_TYPE_IVEC2;
  else if (token.type == TOKEN_KWORD_IVEC3) node->type = VAR_TYPE_IVEC3;
  else if (token.type == TOKEN_KWORD_IVEC4) node->type = VAR_TYPE_IVEC4;
  else if (token.type == TOKEN_KWORD_VEC2) node->type = VAR_TYPE_VEC2;
  else if (token.type == TOKEN_KWORD_VEC3) node->type = VAR_TYPE_VEC3;
  else if (token.type == TOKEN_KWORD_VEC4) node->type = VAR_TYPE_VEC4;
  else if (token.type == TOKEN_KWORD_MAT2) node->type = VAR_TYPE_MAT2;
  else if (token.type == TOKEN_KWORD_MAT3) node->type = VAR_TYPE_MAT3;
  else if (token.type == TOKEN_KWORD_MAT4) node->type = VAR_TYPE_MAT4;
  else if (token.type == TOKEN_KWORD_SAMPLER_2D) node->type = VAR_TYPE_SAMPLER_2D;
  else if (token.type == TOKEN_KWORD_SAMPLER_2D_SHADOW) node->type = VAR_TYPE_SAMPLER_2D_SHADOW;
  else {
    Error("%s: Expect type, got '%s'", token.location.ToString().GetCString(), token.text.GetCString());
    return false;
  }
  node->num = 1;
  
  token = _lexer.Peek();
  if (token.type == TOKEN_LBRACKET) {
    SL_CHECK(Match(TOKEN_LBRACKET));
    Token num_token;
    SL_CHECK(Match(TOKEN_INTEGER, &num_token));
    node->num = num_token.ToInt();
    SL_CHECK(Match(TOKEN_RBRACKET));
  }
  return true;
}

/* VarRef TOKEN_LBRACKET TOKEN_NUMBER TOKEN_RBRACKET
 | VarRef TOKEN_DOT TOKEN_ID
 | TOKEN_ID
 eliminate left recursion --> TOKEN_ID (TOKEN_LBRACKET ArithExpression TOKEN_RBRACKET | TOKEN_DOT TOKEN_ID)*
 */
bool SLParser::VarRef(VarRefNode*& node) {
  Token id;
  SL_CHECK(Match(TOKEN_ID, &id));
  AllocNode(node, NODE_TYPE_VAR_REF);
  node->suffix = nullptr;
  VarRefNode** p_var_ref = &node;
  (*p_var_ref)->ref_type = VAR_REF_TYPE_ID;
  (*p_var_ref)->var_name = CloneToken(&id);
  while (true) {
    auto peek_token = _lexer.Peek();
    if (peek_token.type == TOKEN_LBRACKET) {
      p_var_ref = &(*p_var_ref)->suffix;
      AllocNode(*p_var_ref, NODE_TYPE_VAR_REF);
      (*p_var_ref)->ref_type = VAR_REF_TYPE_INDEX;
      SL_CHECK(Match(TOKEN_LBRACKET));
      SL_CHECK(ArithExpression((*p_var_ref)->index));
      SL_CHECK(Match(TOKEN_RBRACKET));
    } else if (peek_token.type == TOKEN_DOT) {
      p_var_ref = &(*p_var_ref)->suffix;
      AllocNode(*p_var_ref, NODE_TYPE_VAR_REF);
      (*p_var_ref)->ref_type = VAR_REF_TYPE_FIELD;
      SL_CHECK(Match(TOKEN_DOT));
      Token var_name;
      SL_CHECK(Match(TOKEN_ID, &var_name));
      (*p_var_ref)->var_name = CloneToken(&var_name);
    } else break;
  }
  return true;
}

bool SLParser::Expression(ExpressionNode*& node) {
  return OrExpression(node);
}

bool SLParser::OrExpression(ExpressionNode*& node) {
  SL_CHECK(AndExpression(node));
  while (true) {
    auto op_token = _lexer.Peek();
    if (op_token.type == TOKEN_OR) {
      ExpressionNode* first = node;
      AllocNode(node, NODE_TYPE_EXPRESSION);
      node->expr_type = EXPR_TYPE_OR;
      node->location = _lexer.Peek().location;
      node->first = first;
      _lexer.Next();
      SL_CHECK(AndExpression(node->second));
    } else break;
  }
  return true;
}

bool SLParser::AndExpression(ExpressionNode*& node) {
  SL_CHECK(NotExpression(node));
  while (true) {
    auto op_token = _lexer.Peek();
    if (op_token.type == TOKEN_AND) {
      ExpressionNode* first = node;
      AllocNode(node, NODE_TYPE_EXPRESSION);
      node->location = _lexer.Peek().location;
      node->expr_type = EXPR_TYPE_AND;
      node->first = first;
      _lexer.Next();
      SL_CHECK(NotExpression(node->second));
    } else break;
  }
  return true;
}

bool SLParser::NotExpression(ExpressionNode*& node) {
  if (_lexer.Peek().type != TOKEN_NOT) return RelationExpression(node);
  else {
    AllocNode(node, NODE_TYPE_EXPRESSION);
    node->location = _lexer.Peek().location;
    node->expr_type = EXPR_TYPE_NOT;
    _lexer.Next();
    return RelationExpression(node->first);
  }
}

bool SLParser::RelationExpression(ExpressionNode*& node) {
  SL_CHECK(ArithExpression(node));
  auto op_token = _lexer.Peek();
  ExpressionType expr_type = EXPR_TYPE_INVALID;
  if (CheckRelationOp(op_token, expr_type)) {
    ExpressionNode* first = node;
    AllocNode(node, NODE_TYPE_EXPRESSION);
    node->location = _lexer.Peek().location;
    node->expr_type = expr_type;
    node->first = first;
    _lexer.Next();
    SL_CHECK(ArithExpression(node->second));
  }
  return true;

}

bool SLParser::ArithExpression(ExpressionNode*& node) {
  return SumExpression(node);
}

// ProductExpression ((TOKEN_PLUS | TOKEN_MINUS) ProductExpression)*
bool SLParser::SumExpression(ExpressionNode*& node) {
  SL_CHECK(ProductExpression(node));
  while (true) {
    auto op_token = _lexer.Peek();
    if ((op_token.type == TOKEN_PLUS) | (op_token.type == TOKEN_MINUS)) {
      ExpressionNode* first = node;
      AllocNode(node, NODE_TYPE_EXPRESSION);
      node->location = _lexer.Peek().location;
      node->expr_type = op_token.type == TOKEN_PLUS ? EXPR_TYPE_ADD : EXPR_TYPE_MINUS;
      node->first = first;
      _lexer.Next();
      SL_CHECK(ProductExpression(node->second));
    } else break;
  }
  return true;
}

// PrefixExpression ((TOKEN_MUL | TOKEN_DIV) PrefixExpression)*
bool SLParser::ProductExpression(ExpressionNode*& node) {
  SL_CHECK(PrefixExpression(node));
  while (true) {
    auto op_token = _lexer.Peek();
    if ((op_token.type == TOKEN_MUL) | (op_token.type == TOKEN_DIV)) {
      ExpressionNode* first = node;
      AllocNode(node, NODE_TYPE_EXPRESSION);
      node->location = _lexer.Peek().location;
      node->expr_type = op_token.type == TOKEN_MUL ? EXPR_TYPE_MUL : EXPR_TYPE_DIV;
      node->first = first;
      _lexer.Next();
      SL_CHECK(PrefixExpression(node->second));
    } else break;
  }
  return true;
}

// (TOKEN_MINUS | TOKEN_PLUS_PLUS | TOKEN_MINUS_MINUS) ValueExpression
bool SLParser::PrefixExpression(ExpressionNode*& node) {
  auto token = _lexer.Peek();
  if (token.type == TOKEN_MINUS) {
    AllocNode(node, NODE_TYPE_EXPRESSION);
    node->location = _lexer.Peek().location;
    node->expr_type = EXPR_TYPE_NEG;
    _lexer.Next();
    return PostfixExpression(node->first);
  } else if (token.type == TOKEN_PLUS_PLUS) {
    AllocNode(node, NODE_TYPE_EXPRESSION);
    node->location = _lexer.Peek().location;
    node->expr_type = EXPR_TYPE_PRE_INC;
    _lexer.Next();
    return PostfixExpression(node->first);
  } else if (token.type == TOKEN_MINUS_MINUS) {
    AllocNode(node, NODE_TYPE_EXPRESSION);
    node->location = _lexer.Peek().location;
    node->expr_type = EXPR_TYPE_PRE_DEC;
    _lexer.Next();
    return PostfixExpression(node->first);
  } else return PostfixExpression(node);
}

// PrimaryExpression (TOKEN_DOT TOKEN_ID | TOKEN_LBRACKET ArithExpression TOKEN_BRACKET)*
bool SLParser::PostfixExpression(ExpressionNode*& node) {
  SL_CHECK(PrimaryExpression(node));
  auto peek_token = _lexer.Peek();
  while (true) {
    auto first = node;
    if (peek_token.type == TOKEN_DOT) {
      AllocNode(node, NODE_TYPE_EXPRESSION);
      node->location = _lexer.Peek().location;
      node->expr_type = EXPR_TYPE_SUB_FIELD;
      node->sub_field.first = first;
      _lexer.Next();
      Token field_name;
      SL_CHECK(Match(TOKEN_ID, &field_name));
      node->sub_field.field_name = CloneToken(&field_name);
    } else if (peek_token.type == TOKEN_LBRACKET) {
      AllocNode(node, NODE_TYPE_EXPRESSION);
      node->location = _lexer.Peek().location;
      node->expr_type = EXPR_TYPE_SUB_INDEX;
      node->first = node;
      _lexer.Next();
      SL_CHECK(ArithExpression(node->second));
    } else break;
    peek_token = _lexer.Peek();
  } 
  return true;
}

/* TypeDecl TOKEN_LPARENT Expression* TOKEN_RPARENT
 | TOKEN_ID TOKEN_LPARENT Expression* TOKEN_RPARENT 
 | TOKEN_NUMBER
 | TOKEN_INTEGER
 | TOKEN_KWORD_TRUE
 | TOKEN_KWORD_FALSE
 | TOKEN_LPAREN Expression TOKEN_RPARENT
 | VarRef 
 */
bool SLParser::PrimaryExpression(ExpressionNode*& node) {
  auto peek_token = _lexer.Peek();
  if (peek_token.IsDataType()) {
    AllocNode(node, NODE_TYPE_EXPRESSION);
    node->location = _lexer.Peek().location;
    node->expr_type = EXPR_TYPE_CAST;
    SL_CHECK(TypeDecl(node->type_cast.target_type));
    memset(node->type_cast.argv, 0, sizeof(node->type_cast.argv));
    node->type_cast.argc = 0;
    SL_CHECK(Match(TOKEN_LPAREN));
    if (_lexer.Peek().type != TOKEN_RPAREN) {
      int n = 0;
      while (true) {
        SL_CHECK(Expression(node->type_cast.argv[n]));
        ++n;
        peek_token = _lexer.Peek();
        if (peek_token.type == TOKEN_RPAREN) break;
        SL_CHECK(Match(TOKEN_COMMA));
      }
      node->type_cast.argc = n;
    }
    SL_CHECK(Match(TOKEN_RPAREN));
    return true;
  } else if (SeeFunctionCall()) {
    AllocNode(node, NODE_TYPE_EXPRESSION);
    node->location = _lexer.Peek().location;
    node->expr_type = EXPR_TYPE_FUNC_CALL;
    Token id = _lexer.Next();
    node->func_call.func_name = CloneToken(&id);
    node->func_call.argc = 0;
    memset(node->func_call.argv, 0, sizeof(node->func_call.argv));
    SL_CHECK(Match(TOKEN_LPAREN));
    if (_lexer.Peek().type != TOKEN_RPAREN) {
      int n = 0;
      while (true) {
        SL_CHECK(Expression(node->func_call.argv[n]));
        ++n;
        peek_token = _lexer.Peek();
        if (peek_token.type == TOKEN_RPAREN) break;
        SL_CHECK(Match(TOKEN_COMMA));
      }
      node->func_call.argc = n;
    }
    SL_CHECK(Match(TOKEN_RPAREN));
  } else if (peek_token.type == TOKEN_NUMBER) {
    AllocNode(node, NODE_TYPE_EXPRESSION);
    node->location = _lexer.Peek().location;
    node->expr_type = EXPR_TYPE_NUMBER;
    node->number = CloneToken(&peek_token);
    _lexer.Next();
  } else if (peek_token.type == TOKEN_INTEGER) {
    AllocNode(node, NODE_TYPE_EXPRESSION);
    node->location = _lexer.Peek().location;
    node->expr_type = EXPR_TYPE_INTEGER;
    node->integer = CloneToken(&peek_token);
    _lexer.Next();
  } else if (peek_token.type == TOKEN_KWORD_TRUE || peek_token.type == TOKEN_KWORD_FALSE) {
    AllocNode(node, NODE_TYPE_EXPRESSION);
    node->location = _lexer.Peek().location;
    node->expr_type = EXPR_TYPE_BOOLEAN;
    node->boolean = (peek_token.type == TOKEN_KWORD_TRUE);
    _lexer.Next();
  } else if (peek_token.type == TOKEN_LPAREN) {
    SL_CHECK(Match(TOKEN_LPAREN));
    SL_CHECK(Expression(node));
    SL_CHECK(Match(TOKEN_RPAREN));
  } else {
    AllocNode(node, NODE_TYPE_EXPRESSION);
    node->location = _lexer.Peek().location;
    node->expr_type = EXPR_TYPE_VAR_REF;
    SL_CHECK(VarRef(node->var_ref));
  }
  return true;
}

bool SLParser::StatementBlock(vector<ASTNode*>& stmt_nodes) {
  if (_lexer.Peek().type == TOKEN_LBRACE) {
    _lexer.Next();
    auto token = _lexer.Peek();
    while (token && token.type != TOKEN_RBRACE) {
      ASTNode* stmt = nullptr;
      SL_CHECK(Statement(stmt));
      if (stmt) stmt_nodes.push_back(stmt);
      token = _lexer.Peek();
    }
    SL_CHECK(Match(TOKEN_RBRACE));
  } else {
    ASTNode* stmt = nullptr;
    SL_CHECK(Statement(stmt));
    if (stmt) stmt_nodes.push_back(stmt);
  }
  return true;
}

bool SLParser::Statement(ASTNode*& node) {
  union {
    ASTNode* ast_node;
    VarDeclNode* var_decl;
    AssignStatementNode* assign_stmt;
    DiscardStatementNode* discard_stmt;
    IfStatementNode* if_stmt;
    ForStatementNode* for_stmt;
    ReturnStatementNode* return_stmt;
    ContinueStatementNode* continue_stmt;
    BreakStatementNode* break_stmt;
  };
  ast_node = nullptr;
  auto token = _lexer.Peek();
  if (token.type == TOKEN_KWORD_CONST || token.IsDataType()) SL_CHECK(VarDecl(var_decl));
  else if (token.type == TOKEN_ID) SL_CHECK(AssignStatement(assign_stmt));
  else if (token.type == TOKEN_KWORD_DISCARD) SL_CHECK(DiscardStatement(discard_stmt));
  else if (token.type == TOKEN_KWORD_RETURN) SL_CHECK(ReturnStatement(return_stmt));
  else if (token.type == TOKEN_KWORD_CONTINUE) SL_CHECK(ContinueStatement(continue_stmt));
  else if (token.type == TOKEN_KWORD_BREAK) SL_CHECK(BreakStatement(break_stmt));
  else if (token.type == TOKEN_KWORD_FOR) SL_CHECK(ForStatement(for_stmt));
  else if (token.type == TOKEN_KWORD_IF) SL_CHECK(IfStatement(if_stmt));
  else if (token.type == TOKEN_SEMICOLON) _lexer.Next();
  else {
    Error("Invalid token: %s", token.text.GetCString());
    return false;
  }
  node = ast_node;
  return true;
}

bool SLParser::SeeFunctionCall() {
  if (_lexer.Peek().type != TOKEN_ID) return false;
  int pos = _lexer.Save();
  _lexer.Next();
  bool is_func_call = (_lexer.Peek().type == TOKEN_LPAREN);
  _lexer.Restore(pos);
  return is_func_call;
}

void SLParser::FetchFuncParamFlag(int* flags) {
  auto token = _lexer.Peek();
  if (token.type == TOKEN_KWORD_IN) {
    *flags = FUNC_PARAM_IN;
    _lexer.Next();
  } else if (token.type == TOKEN_KWORD_OUT) {
    *flags = FUNC_PARAM_OUT;
    _lexer.Next();
  } else if (token.type == TOKEN_KWORD_INOUT) {
    *flags = FUNC_PARAM_INOUT;
    _lexer.Next();
  } else *flags = FUNC_PARAM_IN;
}

VarType SLParser::GetVarType(TokenType type) const {
  switch (type) {
  case TOKEN_KWORD_BOOL:
    return VAR_TYPE_BOOL;
  case TOKEN_KWORD_INT:
    return VAR_TYPE_INT;
  case TOKEN_KWORD_FLOAT:
    return VAR_TYPE_FLOAT;
  case TOKEN_KWORD_VEC2:
    return VAR_TYPE_VEC2;
  case TOKEN_KWORD_VEC3:
    return VAR_TYPE_VEC3;
  case TOKEN_KWORD_VEC4:
    return VAR_TYPE_VEC4;
  case TOKEN_KWORD_IVEC2:
    return VAR_TYPE_IVEC2;
  case TOKEN_KWORD_IVEC3:
    return VAR_TYPE_IVEC3;
  case TOKEN_KWORD_IVEC4:
    return VAR_TYPE_IVEC4;
  case TOKEN_KWORD_MAT2:
    return VAR_TYPE_MAT2;
  case TOKEN_KWORD_MAT3:
    return VAR_TYPE_MAT3;
  case TOKEN_KWORD_MAT4:
    return VAR_TYPE_MAT4;
  default:
    return VAR_TYPE_VOID;
  }
}

bool SLParser::CheckRelationOp(const Token& token, ExpressionType& out_expr_type) {
  switch (token.type) {
  case TOKEN_EQ:
    out_expr_type = EXPR_TYPE_EQ;
    return true;
  case TOKEN_NEQ:
    out_expr_type = EXPR_TYPE_NEQ;
    return true;
  case TOKEN_LESS:
    out_expr_type = EXPR_TYPE_LESS;
    return true;
  case TOKEN_LEQ:
    out_expr_type = EXPR_TYPE_LEQ;
    return true;
  case TOKEN_GREATER:
    out_expr_type = EXPR_TYPE_GREATER;
    return true;
  case TOKEN_GEQ:
    out_expr_type = EXPR_TYPE_GEQ;
    return true;
  default:
    return false;
  }
}

void SLParser::Error(const char* fmt, ...) {
  static char buf[4096];
  va_list ap;
  va_start(ap, fmt);
  vsprintf(buf, fmt, ap);
  _err_msg = buf;
  fputs(buf, stderr);
  fputc('\n', stderr);
  va_end(ap);
}
