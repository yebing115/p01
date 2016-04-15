#pragma once
#include "SLPreprocessor.h"
#include "SLLexer.h"
#include "Graphics/GraphicsTypes.h"

#define SL_MAX_FUNC_PARAMS 8

enum NodeType {
  NODE_TYPE_INVALID = -1,
  NODE_TYPE_SHADER,
  NODE_TYPE_INPUT_DECL,
  NODE_TYPE_OUTPUT_DECL,
  NODE_TYPE_UNIFORM_DECL,
  NODE_TYPE_TYPE_DECL,
  NODE_TYPE_VAR_DECL,
  NODE_TYPE_FUNC_DECL,
  NODE_TYPE_VAR_REF,
  NODE_TYPE_EXPRESSION,
  NODE_TYPE_ASSIGN_STMT,
  NODE_TYPE_IF_STMT,
  NODE_TYPE_FOR_STMT,
  NODE_TYPE_DISCARD_STMT,
  NODE_TYPE_RETURN_STMT,
  NODE_TYPE_CONTINUE_STMT,
  NODE_TYPE_BREAK_STMT,
  NODE_TYPE_COUNT,
};

enum VarType {
  VAR_TYPE_VOID,
  VAR_TYPE_INT,
  VAR_TYPE_FLOAT,
  VAR_TYPE_BOOL,
  VAR_TYPE_IVEC2,
  VAR_TYPE_IVEC3,
  VAR_TYPE_IVEC4,
  VAR_TYPE_VEC2,
  VAR_TYPE_VEC3,
  VAR_TYPE_VEC4,
  VAR_TYPE_MAT2,
  VAR_TYPE_MAT3,
  VAR_TYPE_MAT4,
  VAR_TYPE_SAMPLER_2D,
  VAR_TYPE_SAMPLER_2D_SHADOW,
  VAR_TYPE_COUNT,
};

struct VarDeclNode;
struct FuncDeclNode;
struct AssignStatementNode;
struct DiscardStatementNode;
struct IfStatementNode;
struct ForStatementNode;
struct ContinueStatementNode;
struct BreakStatementNode;
struct VarRefNode;
struct ExpressionNode;

struct ASTNode {
  NodeType node_type;
};

struct TypeDeclNode;

struct TypeDeclNode: public ASTNode {
  VarType type;
  bool is_const;
  int num;
};

struct VarDeclNode: public ASTNode {
  TypeDeclNode* type_decl;
  Token var_name;
  ExpressionNode* init;
  u16 loc; // annotated by writer(constant location)
};

enum FuncParamFlag {
  FUNC_PARAM_IN = 1,
  FUNC_PARAM_OUT = 2,
  FUNC_PARAM_INOUT = 3,
};

struct FuncDeclNode : public ASTNode {
  TypeDeclNode* ret_type;
  Token func_name;
  struct {
    VarDeclNode* var_decl;
    int flags;
  } params[SL_MAX_FUNC_PARAMS];
  int num_params;
  vector<ASTNode*> statements;
};

struct ShaderNode: public ASTNode {
  ShaderType shader_type;
  vector<VarDeclNode*> inputs;
  vector<VarDeclNode*> outputs;
  vector<VarDeclNode*> uniforms;
  vector<VarDeclNode*> var_decls;
  vector<FuncDeclNode*> func_decls;
  u32 cbuffer_size; // annotated by writer
};

enum ExpressionType {
  EXPR_TYPE_INVALID = -1,
  EXPR_TYPE_INTEGER,
  EXPR_TYPE_NUMBER,
  EXPR_TYPE_BOOLEAN,
  EXPR_TYPE_VAR_REF,
  EXPR_TYPE_SUB_INDEX,
  EXPR_TYPE_SUB_FIELD,
  EXPR_TYPE_ADD,
  EXPR_TYPE_MINUS,
  EXPR_TYPE_MUL,
  EXPR_TYPE_VEC_MUL,
  EXPR_TYPE_DIV,
  EXPR_TYPE_NEG,
  EXPR_TYPE_PRE_INC,
  EXPR_TYPE_PRE_DEC,
  EXPR_TYPE_CAST,
  EXPR_TYPE_EQ,
  EXPR_TYPE_NEQ,
  EXPR_TYPE_LESS,
  EXPR_TYPE_LEQ,
  EXPR_TYPE_GREATER,
  EXPR_TYPE_GEQ,
  EXPR_TYPE_AND,
  EXPR_TYPE_OR,
  EXPR_TYPE_NOT,
  EXPR_TYPE_FUNC_CALL,
  EXPR_TYPE_ASSIGN,
  EXPR_TYPE_PLUS_ASSIGN,
  EXPR_TYPE_MINUS_ASSIGN,
  EXPR_TYPE_MUL_ASSIGN,
  EXPR_TYPE_DIV_ASSIGN,
  EXPR_TYPE_COUNT,
};

struct AssignStatementNode: public ASTNode {
  VarRefNode* left;
  ExpressionType assign_type;
  ExpressionNode* right;
};

struct DiscardStatementNode : public ASTNode {};
struct ContinueStatementNode : public ASTNode {};
struct BreakStatementNode : public ASTNode {};

struct IfStatementNode : public ASTNode {
  ExpressionNode* condition;
  vector<ASTNode*> true_stmts;
  vector<ASTNode*> false_stmts;
};

struct ForStatementNode : public ASTNode {
  VarDeclNode* init;
  ExpressionNode* condition;
  AssignStatementNode* step;
  vector<ASTNode*> statements;
};

struct ReturnStatementNode : public ASTNode {
  ExpressionNode* ret_value;
};

enum VarRefType {
  VAR_REF_TYPE_INVALID = -1,
  VAR_REF_TYPE_ID,
  VAR_REF_TYPE_FIELD,
  VAR_REF_TYPE_INDEX,
};

struct VarRefNode : public ASTNode {
  VarRefType ref_type;
  union {
    Token* var_name;
    Token* field_name;
    ExpressionNode* index;
  };
  VarRefNode* suffix;
};

enum FunctionAnnotation {
  FUNCTION_USER,
  FUNCTION_SAMPLE_2D_SHADOW,
};

struct ExpressionNode : public ASTNode {
  ExpressionType expr_type;
  FileLocation location;
  union {
    VarRefNode* var_ref;
    Token* number;
    Token* integer;
    bool boolean;
    struct {
      TypeDeclNode* target_type;
      ExpressionNode* argv[16];
      int argc;
    } type_cast;
    struct {
      Token* func_name;
      ExpressionNode* argv[16];
      int argc;
      FunctionAnnotation annotation;
    } func_call;
    struct {
      ExpressionNode* first;
      Token* field_name;
    } sub_field;
    struct {
      ExpressionNode* first;
      ExpressionNode* second;
    };
  };
};

class SLParser {
public:
  SLParser();
  ~SLParser();

  bool ParseFile(const String& filename);
  String GetErrorMessage() const { return _err_msg; }
  ShaderNode* GetShader() const { return _shader; }
  void AddDefine(const String& define) { _prep.AddDefine(define); }

private:
  bool Match(TokenType type, Token* token = nullptr) { return _lexer.Match(type, token); }
  bool Shader(ShaderNode*& node, ShaderType shader_type);
  bool InputDecl(VarDeclNode*& node);
  bool OutputDecl(VarDeclNode*& node);
  bool UniformDecl(VarDeclNode*& node);
  bool VarOrFuncDecl(ASTNode*& node);
  bool VarDecl(VarDeclNode*& node, bool match_semicolon = true);
  bool FuncDecl(FuncDeclNode*& node);
  bool AssignStatement(AssignStatementNode*& node, bool match_semicolon = true);
  bool DiscardStatement(DiscardStatementNode*& node);
  bool ContinueStatement(ContinueStatementNode*& node);
  bool BreakStatement(BreakStatementNode*& node);
  bool ReturnStatement(ReturnStatementNode*& node);
  bool IfStatement(IfStatementNode*& node);
  bool ElifStatement(IfStatementNode*& node);
  bool ForStatement(ForStatementNode*& node);
  bool TypeDecl(TypeDeclNode*& node);
  bool VarRef(VarRefNode*& node);
  bool Expression(ExpressionNode*& node);
  bool OrExpression(ExpressionNode*& node);
  bool AndExpression(ExpressionNode*& node);
  bool NotExpression(ExpressionNode*& node);
  bool RelationExpression(ExpressionNode*& node);
  bool ArithExpression(ExpressionNode*& node);
  bool SumExpression(ExpressionNode*& node);
  bool ProductExpression(ExpressionNode*& node);
  bool PrefixExpression(ExpressionNode*& node);
  bool PostfixExpression(ExpressionNode*& node);
  bool PrimaryExpression(ExpressionNode*& node);
  bool StatementBlock(vector<ASTNode*>& stmt_nodes);
  bool Statement(ASTNode*& node);
  bool SeeFunctionCall();
  void FetchFuncParamFlag(int* flags);
  VarType GetVarType(TokenType type) const;
  bool CheckRelationOp(const Token& token, ExpressionType& out_expr_type);
  template <typename T>
  void AllocNode(T*& ptr, NodeType type) {
    ptr = new T();
    ptr->node_type = type;
  }
  Token* CloneToken(Token* token) const { return new Token(*token); }
  void Error(const char* fmt, ...);
  
  SLPreprocessor _prep;
  SLLexer _lexer;
  String _err_msg;
  ShaderNode* _shader;
};

extern const char* VAR_TYPE_NAMES[VAR_TYPE_COUNT];