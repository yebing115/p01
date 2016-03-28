#pragma once
#include "SLParser.h"

enum SymbolFlag {
  SYMBOL_FLAG_INVALID = 0,
  SYMBOL_FLAG_INPUT = 1,
  SYMBOL_FLAG_OUTPUT = 2,
  SYMBOL_FLAG_UNIFORM = 4,
  SYMBOL_FLAG_VAR_NAME = 8,
  SYMBOL_FLAG_FUNC_NAME = 16,
  SYMBOL_FLAG_READABLE = 32,
  SYMBOL_FLAG_WRITABLE = 64,
};

struct ValueType {
  VarType type;
  bool is_const;
  int num;

  ValueType(): type(VAR_TYPE_VOID), is_const(false), num(1) {}
  ValueType(VarType t, int n = 1): type(t), is_const(false), num(n) {}
  ValueType(bool c, VarType t, int n = 1): type(t), is_const(c), num(n) {}
  bool operator ==(const ValueType& o) const { return Equal(o); }
  bool operator !=(const ValueType& o) const { return !Equal(o); }
  bool ExactEqual(const ValueType& o) const { return type == o.type && num == o.num && is_const == o.is_const; }
  bool Equal(const ValueType& o) const { return type == o.type && num == o.num; }
  String ToString() const;
  static const ValueType Sampler2D;
  static const ValueType Void;
  static const ValueType Bool;
  static const ValueType Int;
  static const ValueType Float;
  static const ValueType Vec2;
  static const ValueType Vec3;
  static const ValueType Vec4;
  static const ValueType IVec2;
  static const ValueType IVec3;
  static const ValueType IVec4;
  static const ValueType Mat2;
  static const ValueType Mat3;
  static const ValueType Mat4;
};

struct Symbol {
  String name;
  int flags;
  ValueType type;
  int num;
  FileLocation location;

  Symbol(): flags(0) {}
};

class SLAnalyser {
public:
  SLAnalyser();
  ~SLAnalyser();
  bool TypeCheck(ShaderNode* shader);
  bool TypeCheck(vector<ASTNode*>& statements);
  bool TypeCheck(AssignStatementNode* stmt);
  bool TypeCheck(IfStatementNode* stmt);
  bool TypeCheck(ForStatementNode* stmt);
  bool TypeCheck(ReturnStatementNode* stmt);
  bool TypeCheck(ContinueStatementNode* stmt);
  bool TypeCheck(BreakStatementNode* stmt);
  bool TypeCheck(VarDeclNode* var_decl);
  bool TypeCheckDecl(FuncDeclNode* func_decl);
  bool TypeCheckBody(FuncDeclNode* func_decl);
  void PushSymbolTable();
  void PopSymbolTable();
  void Error(const char* fmt, ...);
private:
  void AllocSymbol(Symbol*& sym, Token* token = nullptr);
  bool SymbolExists(const String& name);
  Symbol* GetSymbol(const String& name);
  bool SymbolTypeMatch(const String& name, ValueType type);
  bool EvalType(TypeDeclNode* type_decl, ValueType* type);
  bool EvalType(VarRefNode* var_ref, ValueType* type);
  bool EvalType(ExpressionNode* expr, ValueType* type);
  bool EvalType(const ValueType& first_type, const String& field, ValueType* out_type);
  bool EvalType(const FileLocation& location, const String& func_name,
                const vector<ValueType>& arg_types, ValueType* out_type);
  void AddBuiltinSymbol(const String& name, const ValueType& type);
  FuncDeclNode* GetFuncDecl(const String& name);
  struct SymbolTableGuard {
    SLAnalyser* _analyer;

    SymbolTableGuard(SLAnalyser* a): _analyer(a) { _analyer->PushSymbolTable(); }
    ~SymbolTableGuard() { _analyer->PopSymbolTable(); }
  };
  typedef unordered_map<String, Symbol*> SymbolTable;
  SymbolTable _symtab;
  vector<SymbolTable> _symtab_stack;
  ValueType _current_ret_type;
  int _loop_depth;
  ShaderNode* _shader;
};

typedef bool(*BuiltinFuncTypeCheckFn)(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type);
struct BuiltinFunc {
  String name;
  BuiltinFuncTypeCheckFn fn;
  
  BuiltinFunc(): fn(nullptr) {}
  BuiltinFunc(const String& name_, BuiltinFuncTypeCheckFn fn_): name(name_), fn(fn_) {}
};

extern BuiltinFunc SL_BUILTIN_FUNCS[];
extern SLAnalyser* g_analyser;
void AddBuiltinSymbol(const String& name, const ValueType& type);
