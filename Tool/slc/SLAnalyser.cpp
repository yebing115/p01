#include "SLAnalyser.h"
#include <stdarg.h>

#define SL_CHECK(x) \
  do { if (!(x)) { Error("%s(%d): %s", __FUNCTION__, __LINE__, #x); return false; } } while (0)

const char* VAR_TYPE_STRING[VAR_TYPE_COUNT] = {
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

const ValueType ValueType::Sampler2D{VAR_TYPE_SAMPLER_2D, 1};
const ValueType ValueType::Sampler2DShadow{VAR_TYPE_SAMPLER_2D_SHADOW, 1};
const ValueType ValueType::Void{VAR_TYPE_VOID, 1};
const ValueType ValueType::Bool{VAR_TYPE_BOOL, 1};
const ValueType ValueType::Int{VAR_TYPE_INT, 1};
const ValueType ValueType::Float{VAR_TYPE_FLOAT, 1};
const ValueType ValueType::Vec2{VAR_TYPE_VEC2, 1};
const ValueType ValueType::Vec3{VAR_TYPE_VEC3, 1};
const ValueType ValueType::Vec4{VAR_TYPE_VEC4, 1};
const ValueType ValueType::IVec2{VAR_TYPE_IVEC2, 1};
const ValueType ValueType::IVec3{VAR_TYPE_IVEC3, 1};
const ValueType ValueType::IVec4{VAR_TYPE_IVEC4, 1};
const ValueType ValueType::Mat2{VAR_TYPE_MAT2, 1};
const ValueType ValueType::Mat3{VAR_TYPE_MAT3, 1};
const ValueType ValueType::Mat4{VAR_TYPE_MAT4, 1};

String ValueType::ToString() const {
  char buf[64];
  String s(VAR_TYPE_STRING[type]);
  if (is_const) s.Prepend("const ");
  if (num > 1) {
    _itoa(num, buf, 10);
    s.Append('[').Append(buf).Append(']');
  }
  return s;
}

SLAnalyser* g_analyser = nullptr;

SLAnalyser::SLAnalyser(): _loop_depth(0), _shader(nullptr) { g_analyser = this; }
SLAnalyser::~SLAnalyser() { g_analyser = nullptr; }

bool SLAnalyser::TypeCheck(ShaderNode* shader) {
  _shader = shader;
  AddBuiltinSymbol("POSITION", ValueType::Vec4);
  for (auto var : shader->inputs) {
    SL_CHECK(!SymbolExists(var->var_name.text));
    Symbol* sym;
    AllocSymbol(sym, &var->var_name);
    sym->flags = SYMBOL_FLAG_INPUT | SYMBOL_FLAG_READABLE;
    SL_CHECK(EvalType(var->type_decl, &sym->type));
    _symtab.insert(make_pair(sym->name, sym));
  }
  for (auto var : shader->outputs) {
    SL_CHECK(!SymbolExists(var->var_name.text));
    Symbol* sym;
    AllocSymbol(sym, &var->var_name);
    sym->flags = SYMBOL_FLAG_OUTPUT | SYMBOL_FLAG_READABLE | SYMBOL_FLAG_WRITABLE;
    SL_CHECK(EvalType(var->type_decl, &sym->type));
    _symtab.insert(make_pair(sym->name, sym));
  }
  for (auto var : shader->uniforms) {
    SL_CHECK(!SymbolExists(var->var_name.text));
    Symbol* sym;
    AllocSymbol(sym, &var->var_name);
    sym->flags = SYMBOL_FLAG_UNIFORM | SYMBOL_FLAG_READABLE;
    SL_CHECK(EvalType(var->type_decl, &sym->type));
    _symtab.insert(make_pair(sym->name, sym));
  }
  for (auto var_decl : shader->var_decls) {
    SL_CHECK(TypeCheck(var_decl));
  }
  for (auto func_decl : shader->func_decls) {
    SL_CHECK(TypeCheckDecl(func_decl));
  }
  for (auto func_decl : shader->func_decls) {
    SL_CHECK(TypeCheckBody(func_decl));
  }
  return true;
}

bool SLAnalyser::TypeCheck(vector<ASTNode*>& statements) {
  bool ok = true;
  for (auto stmt : statements) {
    if (stmt->node_type == NODE_TYPE_ASSIGN_STMT) ok &= TypeCheck((AssignStatementNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_VAR_DECL) ok &= TypeCheck((VarDeclNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_FUNC_DECL) ok &= TypeCheck((VarDeclNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_IF_STMT) ok &= TypeCheck((IfStatementNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_FOR_STMT) ok &= TypeCheck((ForStatementNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_RETURN_STMT) ok &= TypeCheck((ReturnStatementNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_BREAK_STMT) ok &= TypeCheck((BreakStatementNode*)stmt);
    else if (stmt->node_type == NODE_TYPE_CONTINUE_STMT) ok &= TypeCheck((ContinueStatementNode*)stmt);
  }
  return ok;
}

bool SLAnalyser::TypeCheck(AssignStatementNode* stmt) {
  ValueType left_type, right_type;
  if (stmt->assign_type == EXPR_TYPE_PRE_INC || stmt->assign_type == EXPR_TYPE_PRE_DEC) {
    SL_CHECK(EvalType(stmt->right, &right_type));
    if (right_type != ValueType::Int && right_type != ValueType::Float) {
      Error("%s: Not able to increment/decrement type '%s'", stmt->right->location.ToString().GetCString(),
            right_type.ToString().GetCString());
      return false;
    }
  } else {
    SL_CHECK(EvalType(stmt->left, &left_type));
    auto sym = GetSymbol(stmt->left->var_name->text);
    SL_CHECK(sym->flags & SYMBOL_FLAG_WRITABLE);
    SL_CHECK(EvalType(stmt->right, &right_type));
    if (left_type != right_type) {
      Error("%s: Incompatible assignment type '%s', '%s'", stmt->left->var_name->location.ToString().GetCString(),
            left_type.ToString().GetCString(), right_type.ToString().GetCString());
      return false;
    }
  }
  return true;
}

bool SLAnalyser::TypeCheck(IfStatementNode* stmt) {
  ValueType condition_type;
  SL_CHECK(EvalType(stmt->condition, &condition_type));
  SL_CHECK(condition_type == ValueType::Bool);
  if (!stmt->true_stmts.empty()) {
    SymbolTableGuard guard(this);
    SL_CHECK(TypeCheck(stmt->true_stmts));
  }
  if (!stmt->false_stmts.empty()) {
    SymbolTableGuard guard(this);
    SL_CHECK(TypeCheck(stmt->false_stmts));
  }
  return true;
}

bool SLAnalyser::TypeCheck(ForStatementNode* stmt) {
  ValueType condition_type;
  SymbolTableGuard guard(this);
  SL_CHECK(TypeCheck(stmt->init));
  SL_CHECK(EvalType(stmt->condition, &condition_type));
  SL_CHECK(condition_type == ValueType::Bool);
  SL_CHECK(TypeCheck(stmt->step));
  ++_loop_depth;
  SL_CHECK(TypeCheck(stmt->statements));
  --_loop_depth;
  return true;
}

bool SLAnalyser::TypeCheck(VarDeclNode* var_decl) {
  Symbol* sym;
  ValueType type;
  SL_CHECK(EvalType(var_decl->type_decl, &type));
  if (var_decl->init) {
    ValueType init_type;
    SL_CHECK(EvalType(var_decl->init, &init_type));
    if (init_type != type) {
      Error("%s: Variable initialize type incompatible '%s', '%s'.",
            var_decl->var_name.location.ToString().GetCString(),
            type.ToString().GetCString(),
            init_type.ToString().GetCString());
      return false;
    }
  }
  AllocSymbol(sym, &var_decl->var_name);
  sym->type = type;
  sym->flags = SYMBOL_FLAG_VAR_NAME | SYMBOL_FLAG_READABLE | SYMBOL_FLAG_WRITABLE;
  _symtab[sym->name] = sym;
  return true;
}

bool SLAnalyser::TypeCheckDecl(FuncDeclNode* func_decl) {
  for (int i = 0; i < func_decl->num_params; ++i) {
    SL_CHECK(TypeCheck(func_decl->params[i].var_decl));
  }
  Symbol* sym;
  AllocSymbol(sym, &func_decl->func_name);
  sym->flags = SYMBOL_FLAG_FUNC_NAME | SYMBOL_FLAG_READABLE;
  SL_CHECK(EvalType(func_decl->ret_type, &sym->type));
  _symtab.insert(make_pair(sym->name, sym));
  return true;
}

bool SLAnalyser::TypeCheckBody(FuncDeclNode* func_decl) {
  SymbolTableGuard guard(this);
  _current_ret_type = GetSymbol(func_decl->func_name.text)->type;
  SL_CHECK(TypeCheck(func_decl->statements));
  _current_ret_type = ValueType::Void;
  return true;
}

bool SLAnalyser::TypeCheck(ReturnStatementNode* stmt) {
  ValueType ret_type;
  SL_CHECK(EvalType(stmt->ret_value, &ret_type));
  if (ret_type != _current_ret_type) {
    Error("%s: Return type mismatch '%s', expect '%s'.",
          stmt->ret_value->location.ToString().GetCString(),
          ret_type.ToString().GetCString(),
          _current_ret_type.ToString().GetCString());
    return false;
  }
  return true;
}

bool SLAnalyser::TypeCheck(ContinueStatementNode* stmt) {
  SL_CHECK(_loop_depth > 0);
  return true;
}

bool SLAnalyser::TypeCheck(BreakStatementNode* stmt) {
  SL_CHECK(_loop_depth > 0);
  return true;
}

void SLAnalyser::AllocSymbol(Symbol*& sym, Token* token) {
  sym = new Symbol;
  if (token) {
    sym->name = token->text;
    sym->location = token->location;
  }
}

bool SLAnalyser::SymbolExists(const String& name) {
  return (_symtab.find(name) != _symtab.end());
}

Symbol* SLAnalyser::GetSymbol(const String& name) {
  auto it = _symtab.find(name);
  if (it == _symtab.end()) return nullptr;
  return it->second;
}

bool SLAnalyser::SymbolTypeMatch(const String& name, ValueType type) {
  auto it = _symtab.find(name);
  if (it == _symtab.end()) return false;
  return (it->second->type == type);
}

bool SLAnalyser::EvalType(TypeDeclNode* type_decl, ValueType* type) {
  type->type = type_decl->type;
  type->num = type_decl->num;
  return true;
}

bool SLAnalyser::EvalType(VarRefNode* var_ref, ValueType* out_type) {
  ValueType type;
  SL_CHECK(var_ref->ref_type == VAR_REF_TYPE_ID);
  Symbol* sym = GetSymbol(var_ref->var_name->text);
  if (!sym) {
    Error("%s: Symbol '%s' not found.", var_ref->var_name->location.ToString().GetCString(),
          var_ref->var_name->text.GetCString());
    return false;
  }
  type = sym->type;
  var_ref = var_ref->suffix;
  while (var_ref) {
    if (var_ref->ref_type == VAR_REF_TYPE_INDEX) {
      if (type.num > 1) type.num = 1;
      else if (type.type >= VAR_TYPE_VEC2 && type.type <= VAR_TYPE_VEC4) type.type = VAR_TYPE_FLOAT;
      else if (type.type >= VAR_TYPE_IVEC2 && type.type <= VAR_TYPE_IVEC4) type.type = VAR_TYPE_INT;
      else if (type.type == VAR_TYPE_MAT2) type.type = VAR_TYPE_VEC2;
      else if (type.type == VAR_TYPE_MAT3) type.type = VAR_TYPE_VEC3;
      else if (type.type == VAR_TYPE_MAT4) type.type = VAR_TYPE_VEC4;
      else {
        Error("%s: None indexable type: %s", sym->location.ToString().GetCString(), type.ToString().GetCString());
        return false;
      }
    } else if (var_ref->ref_type == VAR_REF_TYPE_FIELD) {
      if (type.num > 1) {
        Error("Sub-field on array type.");
        return false;
      }
      if (type.type >= VAR_TYPE_VEC2 && type.type <= VAR_TYPE_VEC4) {
        auto field_length = var_ref->field_name->text.GetLength();
        if (field_length == 1) type.type = VAR_TYPE_FLOAT;
        else if (field_length == 2) type.type = VAR_TYPE_VEC2;
        else if (field_length == 3) type.type = VAR_TYPE_VEC3;
        else if (field_length == 4) type.type = VAR_TYPE_VEC4;
        else {
          Error("%s: Sub-field '%s' too long.", sym->location.ToString().GetCString(), var_ref->field_name->text.GetCString());
          return false;
        }
      } else if (type.type >= VAR_TYPE_IVEC2 && type.type <= VAR_TYPE_IVEC4) {
        auto field_length = var_ref->field_name->text.GetLength();
        if (field_length == 1) type.type = VAR_TYPE_INT;
        else if (field_length == 2) type.type = VAR_TYPE_IVEC2;
        else if (field_length == 3) type.type = VAR_TYPE_IVEC3;
        else if (field_length == 4) type.type = VAR_TYPE_IVEC4;
        else {
          Error("%s: Sub-field '%s' too long.", sym->location.ToString().GetCString(), var_ref->field_name->text.GetCString());
          return false;
        }
      } else {
        Error("%s: Unable to sub-field type '%s'", sym->location.ToString().GetCString(), type.ToString().GetCString());
        return false;
      }
    } else {
      Error("%s: Invalid type.", sym->location.ToString().GetCString());
      return false;
    }
    var_ref = var_ref->suffix;
  }
  *out_type = type;
  return true;
}

bool SLAnalyser::EvalType(ExpressionNode* expr, ValueType* type) {
  ValueType first_type, second_type;
  if (expr->expr_type == EXPR_TYPE_VAR_REF) return EvalType(expr->var_ref, type);
  else if (expr->expr_type == EXPR_TYPE_INTEGER) {
    type->type = VAR_TYPE_INT;
    type->num = 1;
  } else if (expr->expr_type == EXPR_TYPE_NUMBER) {
    type->type = VAR_TYPE_FLOAT;
    type->num = 1;
  } else if (expr->expr_type == EXPR_TYPE_BOOLEAN) {
    type->type = VAR_TYPE_BOOL;
    type->num = 1;
  } else if (expr->expr_type == EXPR_TYPE_ADD || expr->expr_type == EXPR_TYPE_MINUS ||
             expr->expr_type == EXPR_TYPE_MUL || expr->expr_type == EXPR_TYPE_DIV) {
    SL_CHECK(EvalType(expr->first, &first_type));
    SL_CHECK(EvalType(expr->second, &second_type));
    if ((first_type == ValueType::Int || first_type == ValueType::Float) &&
        (second_type == ValueType::Int || second_type == ValueType::Float)) {
      if (first_type == ValueType::Int && second_type == ValueType::Int) *type = first_type;
      else *type = ValueType::Float;
    } else if (expr->expr_type == EXPR_TYPE_ADD || expr->expr_type == EXPR_TYPE_MINUS) {
      
      if (first_type == ValueType::Int && second_type == ValueType::Int) *type = ValueType::Int;
      else if (first_type == ValueType::Int && second_type == ValueType::Float ||
               first_type == ValueType::Float && second_type == ValueType::Int ||
               first_type == ValueType::Float && second_type == ValueType::Float) *type = ValueType::Float;
      else if (first_type == ValueType::Vec2 && second_type == ValueType::Vec2 ||
               first_type == ValueType::Vec2 && second_type == ValueType::Float ||
               first_type == ValueType::Float && second_type == ValueType::Vec2) *type = ValueType::Vec2;
      else if (first_type == ValueType::Vec3 && second_type == ValueType::Vec3 ||
               first_type == ValueType::Vec3 && second_type == ValueType::Float ||
               first_type == ValueType::Float && second_type == ValueType::Vec3) *type = ValueType::Vec3;
      else if (first_type == ValueType::Vec4 && second_type == ValueType::Vec4 ||
               first_type == ValueType::Vec4 && second_type == ValueType::Float ||
               first_type == ValueType::Float && second_type == ValueType::Vec4) *type = ValueType::Vec4;
      else if (first_type == ValueType::IVec2 && second_type == ValueType::IVec2 ||
               first_type == ValueType::IVec2 && second_type == ValueType::Float ||
               first_type == ValueType::Float && second_type == ValueType::IVec2) *type = ValueType::IVec2;
      else if (first_type == ValueType::IVec3 && second_type == ValueType::IVec3 ||
               first_type == ValueType::IVec3 && second_type == ValueType::Float ||
               first_type == ValueType::Float && second_type == ValueType::IVec3) *type = ValueType::IVec3;
      else if (first_type == ValueType::IVec4 && second_type == ValueType::IVec4 ||
               first_type == ValueType::IVec4 && second_type == ValueType::Float ||
               first_type == ValueType::Float && second_type == ValueType::IVec4) *type = ValueType::IVec4;
      else if (first_type == ValueType::Mat2 && second_type == ValueType::Mat2) *type = ValueType::Mat2;
      else if (first_type == ValueType::Mat3 && second_type == ValueType::Mat3) *type = ValueType::Mat3;
      else if (first_type == ValueType::Mat4 && second_type == ValueType::Mat4) *type = ValueType::Mat4;
      else {
        Error("%s: Not compatible type '%s', '%s'.", expr->location.ToString().GetCString(), first_type.ToString().GetCString(), second_type.ToString().GetCString());
        return false;
      }
    } else {
      if (first_type == ValueType::Int && second_type == ValueType::Int) *type = ValueType::Int;
      else if (first_type == ValueType::Int && second_type == ValueType::Float ||
               first_type == ValueType::Float && second_type == ValueType::Int ||
               first_type == ValueType::Float && second_type == ValueType::Float) *type = ValueType::Float;
      else if (first_type == ValueType::Vec2 && second_type == ValueType::Mat2 ||
               first_type == ValueType::Mat2 && second_type == ValueType::Vec2) {
        expr->expr_type = EXPR_TYPE_VEC_MUL;
        *type = ValueType::Vec2;
      } else if (first_type == ValueType::Vec2 && second_type == ValueType::Vec2 ||
                 first_type == ValueType::Vec2 && second_type == ValueType::Float ||
                 first_type == ValueType::Float && second_type == ValueType::Vec2) *type = ValueType::Vec2;
      else if (first_type == ValueType::Vec3 && second_type == ValueType::Mat3 ||
               first_type == ValueType::Mat3 && second_type == ValueType::Vec3) {
        expr->expr_type = EXPR_TYPE_VEC_MUL;
        *type = ValueType::Vec3;
      } else if (first_type == ValueType::Vec3 && second_type == ValueType::Vec3 ||
                first_type == ValueType::Vec3 && second_type == ValueType::Float ||
                first_type == ValueType::Float && second_type == ValueType::Vec3) *type = ValueType::Vec3;
      else if (first_type == ValueType::Vec4 && second_type == ValueType::Mat4 ||
               first_type == ValueType::Mat4 && second_type == ValueType::Vec4) {
        expr->expr_type = EXPR_TYPE_VEC_MUL;
        *type = ValueType::Vec4;
      } else if (first_type == ValueType::Vec4 && second_type == ValueType::Vec4 ||
                first_type == ValueType::Vec4 && second_type == ValueType::Float ||
                first_type == ValueType::Float && second_type == ValueType::Vec4) *type = ValueType::Vec4;
      else if (first_type == ValueType::IVec2 && second_type == ValueType::IVec2 ||
               first_type == ValueType::IVec2 && second_type == ValueType::Float ||
               first_type == ValueType::Float && second_type == ValueType::IVec2) *type = ValueType::IVec2;
      else if (first_type == ValueType::IVec3 && second_type == ValueType::IVec3 ||
               first_type == ValueType::IVec3 && second_type == ValueType::Float ||
               first_type == ValueType::Float && second_type == ValueType::IVec3) *type = ValueType::IVec3;
      else if (first_type == ValueType::IVec4 && second_type == ValueType::IVec4 ||
               first_type == ValueType::IVec4 && second_type == ValueType::Float ||
               first_type == ValueType::Float && second_type == ValueType::IVec4) *type = ValueType::IVec4;
      else if (first_type == ValueType::Mat2 && second_type == ValueType::Mat2) {
        expr->expr_type = EXPR_TYPE_VEC_MUL;
        *type = ValueType::Mat2;
      } else if (first_type == ValueType::Mat2 && second_type == ValueType::Float ||
                 first_type == ValueType::Float && second_type == ValueType::Mat2) *type = ValueType::Mat2;
      else if (first_type == ValueType::Mat3 && second_type == ValueType::Mat3) {
        expr->expr_type = EXPR_TYPE_VEC_MUL;
        *type = ValueType::Mat3;
      } else if (first_type == ValueType::Float && second_type == ValueType::Mat3 ||
                first_type == ValueType::Mat3 && second_type == ValueType::Float) *type = ValueType::Mat3;
      else if (first_type == ValueType::Mat4 && second_type == ValueType::Mat4) {
        expr->expr_type = EXPR_TYPE_VEC_MUL;
        *type = ValueType::Mat4;
      } else if (first_type == ValueType::Mat4 && second_type == ValueType::Float || 
                first_type == ValueType::Float && second_type == ValueType::Mat4) *type = ValueType::Mat4;
      else {
        Error("%s: Not arithmetic compatible type '%s', '%s'.", expr->location.ToString().GetCString(), first_type.ToString().GetCString(), second_type.ToString().GetCString());
        return false;
      }
    }
  } else if (expr->expr_type == EXPR_TYPE_NEG) {
    SL_CHECK(EvalType(expr->first, &first_type));
    if (first_type == ValueType::Int || first_type == ValueType::Float ||
        first_type == ValueType::Vec2 || first_type == ValueType::Vec3 || first_type == ValueType::Vec4 ||
        first_type == ValueType::IVec2 || first_type == ValueType::IVec3 || first_type == ValueType::IVec4 ||
        first_type == ValueType::Mat2 || first_type == ValueType::Mat3 || first_type == ValueType::Mat4) {
      *type = first_type;
    } else {
      Error("%s: Not negative-able type '%s'.", expr->location.ToString().GetCString(),
            first_type.ToString().GetCString());
      return false;
    }
  } else if (expr->expr_type == EXPR_TYPE_CAST) {
    vector<ValueType> arg_types(expr->type_cast.argc);
    for (int i = 0; i < expr->type_cast.argc; ++i) {
      SL_CHECK(EvalType(expr->type_cast.argv[i], &arg_types[i]));
    }
    SL_CHECK(EvalType(expr->type_cast.target_type, type));
  } else if (expr->expr_type == EXPR_TYPE_EQ || expr->expr_type == EXPR_TYPE_NEQ) {
    SL_CHECK(EvalType(expr->first, &first_type));
    SL_CHECK(EvalType(expr->second, &second_type));
    if (first_type == second_type && first_type.num == 1) *type = ValueType::Bool;
    else {
      Error("%s: Not comparable type '%s', '%s'.", expr->location.ToString().GetCString(),
            first_type.ToString().GetCString(), second_type.ToString().GetCString());
      return false;
    }
  } else if (expr->expr_type == EXPR_TYPE_LESS || expr->expr_type == EXPR_TYPE_LEQ ||
             expr->expr_type == EXPR_TYPE_GREATER || expr->expr_type == EXPR_TYPE_GEQ) {
    SL_CHECK(EvalType(expr->first, &first_type));
    SL_CHECK(EvalType(expr->second, &second_type));
    if (first_type == second_type && (first_type == ValueType::Int || first_type == ValueType::Float)) *type = ValueType::Bool;
    else {
      Error("%s: Not comparable type '%s', '%s'.", expr->location.ToString().GetCString(),
            first_type.ToString().GetCString(), second_type.ToString().GetCString());
      return false;
    }
  } else if (expr->expr_type == EXPR_TYPE_AND || expr->expr_type == EXPR_TYPE_OR) {
    SL_CHECK(EvalType(expr->first, &first_type));
    SL_CHECK(EvalType(expr->second, &second_type));
    if (first_type == ValueType::Bool && second_type == ValueType::Bool) *type = ValueType::Bool;
    else {
      Error("%s: Not bool type '%s', '%s'.", expr->location.ToString().GetCString(),
            first_type.ToString().GetCString(), second_type.ToString().GetCString());
      return false;
    }
  } else if (expr->expr_type == EXPR_TYPE_NOT) {
    SL_CHECK(EvalType(expr->first, &first_type));
    if (first_type == ValueType::Bool) *type = ValueType::Bool;
    else {
      Error("%s: Not bool type '%s'.", expr->location.ToString().GetCString(),
            first_type.ToString().GetCString());
      return false;
    }
  } else if (expr->expr_type == EXPR_TYPE_FUNC_CALL) {
    auto sym = GetSymbol(expr->func_call.func_name->text);
    if (sym && !!(sym->flags & SYMBOL_FLAG_FUNC_NAME)) {
      FuncDeclNode* func_decl = GetFuncDecl(sym->name);
      SL_CHECK(func_decl != nullptr);
      SL_CHECK(expr->func_call.argc == func_decl->num_params);
      for (int i = 0; i < func_decl->num_params; ++i) {
        ValueType arg_type;
        ValueType param_type;
        SL_CHECK(EvalType(expr->func_call.argv[i], &arg_type));
        SL_CHECK(EvalType(func_decl->params[i].var_decl->type_decl, &param_type));
        SL_CHECK(arg_type == param_type);
      }
      *type = sym->type;
    } else if (!sym) {
      vector<ValueType> arg_types(expr->func_call.argc);
      for (int i = 0; i < expr->func_call.argc; ++i) {
        SL_CHECK(EvalType(expr->func_call.argv[i], &arg_types[i]));
      }
      SL_CHECK(EvalType(expr->location, expr->func_call.func_name->text, arg_types, type));
      expr->func_call.annotation = type->func_call_annotation;
    } else {
      Error("%s: Not a function name symbol '%s'", expr->location.ToString().GetCString(), expr->func_call.func_name->text.GetCString());
      return false;
    }
  } else if (expr->expr_type == EXPR_TYPE_SUB_FIELD) {
    SL_CHECK(EvalType(expr->sub_field.first, &first_type));
    bool ok = EvalType(first_type, expr->sub_field.field_name->text, type);
    if (!ok) {
      Error("%s: Not support sub_field type '%s' field '%s'", expr->location.ToString().GetCString(),
            first_type.ToString().GetCString(), expr->sub_field.field_name->text.GetCString());
      return false;
    }
  } else if (expr->expr_type == EXPR_TYPE_SUB_INDEX) {
    SL_CHECK(EvalType(expr->first, &first_type));
    SL_CHECK(EvalType(expr->second, &second_type));
    if (second_type != ValueType::Int && second_type != ValueType::Float) {
      Error("%s: Incompatible index type '%s'", expr->location.ToString().GetCString(),
            second_type.ToString().GetCString());
      return false;
    }
    if (first_type.num == 1) {
      if (first_type == ValueType::Vec2 || first_type == ValueType::Vec3 || first_type == ValueType::Vec4) *type = ValueType::Float;
      else if (first_type == ValueType::IVec2 || first_type == ValueType::IVec3 || first_type == ValueType::IVec4) *type = ValueType::Int;
      else if (first_type == ValueType::Mat2) *type = ValueType::Vec2;
      else if (first_type == ValueType::Mat3) *type = ValueType::Vec3;
      else if (first_type == ValueType::Mat4) *type = ValueType::Vec4;
      else {
        Error("%s: Not support sub_index type '%s'", expr->location.ToString().GetCString(),
              first_type.ToString().GetCString());
        return false;
      }
    } else {
      *type = first_type;
      type->num = 1;
    }
  } else {
    *type = ValueType::Void;
  }
  return true;
}

bool SLAnalyser::EvalType(const ValueType& type, const String& field, ValueType* out_type) {
  int n = field.GetLength();
  int num_elems = 0;
  if (type == ValueType::Vec2 || type == ValueType::Vec3 || type == ValueType::Vec4) {
    num_elems = type.type - VAR_TYPE_VEC2 + 2;
    if (n == 1) *out_type = ValueType::Float;
    else if (n == 2) *out_type = ValueType::Vec2;
    else if (n == 3) *out_type = ValueType::Vec3;
    else if (n == 4) *out_type = ValueType::Vec4;
    else return false;
  } else if (type == ValueType::IVec2 || type == ValueType::IVec3 || type == ValueType::IVec4) {
    num_elems = type.type - VAR_TYPE_IVEC2 + 2;
    if (n == 1) *out_type = ValueType::Int;
    else if (n == 2) *out_type = ValueType::IVec2;
    else if (n == 3) *out_type = ValueType::IVec3;
    else if (n == 4) *out_type = ValueType::IVec4;
    else return false;
  } else return false;
  String allow_field_names[4] = {"rx", "rgxy", "rgbxyz", "rgbaxyzw"};
  auto& allow_name = allow_field_names[num_elems - 1];
  for (int i = 0; i < n; ++i) {
    if (allow_name.Find(field.Substr(i, 1)) == -1) return false;
  }
  return true;
}

bool SLAnalyser::EvalType(const FileLocation& location, const String& func_name,
                          const vector<ValueType>& arg_types, ValueType* out_type) {
  auto func = SL_BUILTIN_FUNCS;
  while (func->fn) {
    if (func->name == func_name) return func->fn(location, arg_types, out_type);
    ++func;
  }
  Error("%s: Not valid function symbol '%s'", location.ToString().GetCString(), func_name.GetCString());
  return false;
}

void SLAnalyser::AddBuiltinSymbol(const String& name, const ValueType& type) {
  Symbol* sym;
  AllocSymbol(sym);
  sym->name = name;
  sym->flags = SYMBOL_FLAG_VAR_NAME | SYMBOL_FLAG_READABLE | SYMBOL_FLAG_WRITABLE;
  sym->location = FileLocation("<builtin>", 1, 1);
  sym->type = type;
  _symtab.insert(make_pair(sym->name, sym));
}

FuncDeclNode* SLAnalyser::GetFuncDecl(const String& name) {
  if (!_shader) return nullptr;
  for (auto func_decl : _shader->func_decls) {
    if (func_decl->func_name.text == name) return func_decl;
  }
  return nullptr;
}

void SLAnalyser::Error(const char* fmt, ...) {
  static char buf[4096];
  va_list ap;
  va_start(ap, fmt);
  vsprintf(buf, fmt, ap);
  fputs(buf, stderr);
  fputc('\n', stderr);
  va_end(ap);
}

void SLAnalyser::PushSymbolTable() {
  _symtab_stack.push_back(_symtab);
}

void SLAnalyser::PopSymbolTable() {
  if (!_symtab_stack.empty()) {
    _symtab = _symtab_stack.back();
    _symtab_stack.pop_back();
  } else {
    _symtab.clear();
  }
}

//////////////////////////////////////////////////////////////////////////

bool type_check_func_abs(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'abs' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types.front();
  return true;
}

bool type_check_func_sign(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'sign' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = ValueType::Float;
  return true;
}

bool type_check_func_min(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'min' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != arg_types[1]) {
    g_analyser->Error("%s: 'min' function argument type mismatch '%s', '%s'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_max(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'max' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != arg_types[1]) {
    g_analyser->Error("%s: 'max' function argument type mismatch '%s', '%s'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_mix(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 3) {
    g_analyser->Error("%s: 'mix' function expect 3 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != arg_types[1]) {
    g_analyser->Error("%s: 'mix' function argument 1 and 2 type mismatch '%s', '%s'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  if (arg_types[2] != ValueType::Float) {
    g_analyser->Error("%s: 'mix' function argument 3 expect 'float', not '%s'.",
                      location.ToString().GetCString(),
                      arg_types[2].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_clamp(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 3) {
    g_analyser->Error("%s: 'clamp' function expect 3 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != arg_types[1] || arg_types[0] != arg_types[2]) {
    g_analyser->Error("%s: 'clamp' function argument type mismatch '%s', '%s', '%s'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString(),
                      arg_types[1].ToString().GetCString(),
                      arg_types[2].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_smoothstep(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 3) {
    g_analyser->Error("%s: 'smoothstep' function expect 3 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != arg_types[1] || arg_types[0] != arg_types[2]) {
    g_analyser->Error("%s: 'smoothstep' function argument type mismatch '%s', '%s', '%s'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString(),
                      arg_types[1].ToString().GetCString(),
                      arg_types[2].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_normalize(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'normalize' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}
bool type_check_func_length(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'length' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = ValueType::Float;
  return true;
}

bool type_check_func_distance(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'distance' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != arg_types[1]) {
    g_analyser->Error("%s: 'distance' function argument type mismatch '%s', '%s'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  *out_type = ValueType::Float;
  return true;
}

bool type_check_func_dot(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'dot' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != arg_types[1]) {
    g_analyser->Error("%s: 'dot' function argument type mismatch '%s', '%s'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  *out_type = ValueType::Float;
  return true;
}

bool type_check_func_cross(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'cross' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != arg_types[1]) {
    g_analyser->Error("%s: 'cross' function argument type mismatch '%s', '%s'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_texture(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2 && arg_types.size() != 3) {
    g_analyser->Error("%s: 'texture' function expect 2 or 3 arguments.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] == ValueType::Sampler2D) {
    if (arg_types[0] != ValueType::Sampler2D) {
    }
    if (arg_types[1] != ValueType::Vec2) {
      g_analyser->Error("%s: 'texture' argument 2 type mismatch '%s', not 'vec2'.",
                        location.ToString().GetCString(),
                        arg_types[1].ToString().GetCString());
      return false;
    }
    if (arg_types.size() == 3 && arg_types[2] != ValueType::Float) {
      g_analyser->Error("%s: 'texture' argument 3 type mismatch '%s', not 'float'.",
                        location.ToString().GetCString(),
                        arg_types[2].ToString().GetCString());
      return false;
    }
    *out_type = ValueType::Vec4;
  } else if (arg_types[0] == ValueType::Sampler2DShadow) {
    if (arg_types[1] != ValueType::Vec3) {
      g_analyser->Error("%s: 'texture' argument 2 type mismatch '%s', not 'vec3'.",
                        location.ToString().GetCString(),
                        arg_types[1].ToString().GetCString());
      return false;
    }
    if (arg_types.size() == 3 && arg_types[2] != ValueType::Float) {
      g_analyser->Error("%s: 'texture' argument 3 type mismatch '%s', not 'float'.",
                        location.ToString().GetCString(),
                        arg_types[2].ToString().GetCString());
      return false;
    }
    *out_type = ValueType::Float;
    out_type->func_call_annotation = FUNCTION_SAMPLE_2D_SHADOW;
  } else {
    g_analyser->Error("%s: 'texture' argument 1 type mismatch '%s', not 'sampler2D'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString());
    return false;
  }

  return true;
}

bool type_check_func_texelFetch(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 3) {
    g_analyser->Error("%s: 'texelFetch' function expect 3 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != ValueType::Sampler2D) {
    g_analyser->Error("%s: 'texelFetch' argument 1 type mismatch '%s', not 'sampler2D'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString());
    return false;
  }
  if (arg_types[1] != ValueType::IVec2) {
    g_analyser->Error("%s: 'texelFetch' argument 2 type mismatch '%s', not 'ivec2'.",
                      location.ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  if (arg_types[2] != ValueType::Int) {
    g_analyser->Error("%s: 'texelFetch' argument 3 type mismatch '%s', not 'int'.",
                      location.ToString().GetCString(),
                      arg_types[2].ToString().GetCString());
    return false;
  }
  *out_type = ValueType::Vec4;
  return true;
}

bool type_check_func_textureSize(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'textureSize' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != ValueType::Sampler2D) {
    g_analyser->Error("%s: 'textureSize' argument 1 type mismatch '%s', not 'sampler2D'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString());
    return false;
  }
  if (arg_types[1] != ValueType::Int) {
    g_analyser->Error("%s: 'textureSize' argument 2 type mismatch '%s', not 'int'.",
                      location.ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  *out_type = ValueType::IVec2;
  return true;
}

bool type_check_func_textureProj(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2 && arg_types.size() != 3) {
    g_analyser->Error("%s: 'textureProj' function expect 2 or 3 arguments.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != ValueType::Sampler2D) {
    g_analyser->Error("%s: 'textureProj' argument 1 type mismatch '%s', not 'sampler2D'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString());
    return false;
  }
  if (arg_types[1] != ValueType::Vec3 && arg_types[1] != ValueType::Vec4) {
    g_analyser->Error("%s: 'textureProj' argument 2 type mismatch '%s', not 'vec3' or 'vec4'.",
                      location.ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  if (arg_types.size() == 3 && arg_types[2] != ValueType::Float) {
    g_analyser->Error("%s: 'textureProj' argument 3 type mismatch '%s', not 'float'.",
                      location.ToString().GetCString(),
                      arg_types[2].ToString().GetCString());
    return false;
  }

  *out_type = ValueType::Vec4;
  return true;
}

bool type_check_func_reflect(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'reflect' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != arg_types[1]) {
    g_analyser->Error("%s: 'reflect' function argument type mismatch '%s', '%s'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_refract(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'refract' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != arg_types[1]) {
    g_analyser->Error("%s: 'refract' function argument type mismatch '%s', '%s'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_step(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'step' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != arg_types[1]) {
    g_analyser->Error("%s: 'step' function argument type mismatch '%s', '%s'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString(),
                      arg_types[1].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_transpose(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'transpose' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != ValueType::Mat2 && arg_types[0] != ValueType::Mat3 && arg_types[0] != ValueType::Mat4) {
    g_analyser->Error("%s: 'transpose' function argument type mismatch '%s', expect 'mat2' or 'mat3' or 'mat4'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_inverse(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'inverse' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != ValueType::Mat2 && arg_types[0] != ValueType::Mat3 && arg_types[0] != ValueType::Mat4) {
    g_analyser->Error("%s: 'inverse' function argument type mismatch '%s', expect 'mat2' or 'mat3' or 'mat4'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_determinant(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'determinant' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  if (arg_types[0] != ValueType::Mat2 && arg_types[0] != ValueType::Mat3 && arg_types[0] != ValueType::Mat4) {
    g_analyser->Error("%s: 'determinant' function argument type mismatch '%s', expect 'mat2' or 'mat3' or 'mat4'.",
                      location.ToString().GetCString(),
                      arg_types[0].ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_sin(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'sin' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_cos(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'cos' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_asin(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'asin' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_acos(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'acos' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_sinh(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'sinh' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_cosh(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'cosh' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_asinh(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'asinh' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_acosh(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'acosh' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_tan(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'tan' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_atan(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1 && arg_types.size() != 2) {
    g_analyser->Error("%s: 'atan' function expect 1 or 2 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_tanh(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'tanh' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_atanh(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'atanh' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_pow(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'pow' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_exp(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'exp' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_log(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'log' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_pow2(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'pow2' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_exp2(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 2) {
    g_analyser->Error("%s: 'exp2' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_log2(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'log2' function expect 1 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

bool type_check_func_fract(const FileLocation& location, const vector<ValueType>& arg_types, ValueType* out_type) {
  if (arg_types.size() != 1) {
    g_analyser->Error("%s: 'fract' function expect 2 argument.", location.ToString().GetCString());
    return false;
  }
  *out_type = arg_types[0];
  return true;
}

#define TYPE_CHECK_FUNC(x) {#x, type_check_func_##x}
BuiltinFunc SL_BUILTIN_FUNCS[] = {
  TYPE_CHECK_FUNC(abs),
  TYPE_CHECK_FUNC(sign),
  TYPE_CHECK_FUNC(min),
  TYPE_CHECK_FUNC(max),
  TYPE_CHECK_FUNC(mix),
  TYPE_CHECK_FUNC(clamp),
  TYPE_CHECK_FUNC(smoothstep),
  TYPE_CHECK_FUNC(step),
  TYPE_CHECK_FUNC(normalize),
  TYPE_CHECK_FUNC(length),
  TYPE_CHECK_FUNC(distance),
  TYPE_CHECK_FUNC(dot),
  TYPE_CHECK_FUNC(cross),
  TYPE_CHECK_FUNC(reflect),
  TYPE_CHECK_FUNC(refract),
  TYPE_CHECK_FUNC(transpose),
  TYPE_CHECK_FUNC(inverse),
  TYPE_CHECK_FUNC(determinant),
  TYPE_CHECK_FUNC(texture),
  TYPE_CHECK_FUNC(textureProj),
  TYPE_CHECK_FUNC(texelFetch),
  TYPE_CHECK_FUNC(textureSize),
  TYPE_CHECK_FUNC(sin),
  TYPE_CHECK_FUNC(cos),
  TYPE_CHECK_FUNC(sinh),
  TYPE_CHECK_FUNC(cosh),
  TYPE_CHECK_FUNC(tan),
  TYPE_CHECK_FUNC(tanh),
  TYPE_CHECK_FUNC(asin),
  TYPE_CHECK_FUNC(acos),
  TYPE_CHECK_FUNC(asinh),
  TYPE_CHECK_FUNC(acosh),
  TYPE_CHECK_FUNC(atan),
  TYPE_CHECK_FUNC(atanh),
  TYPE_CHECK_FUNC(pow),
  TYPE_CHECK_FUNC(exp),
  TYPE_CHECK_FUNC(log),
  TYPE_CHECK_FUNC(pow2),
  TYPE_CHECK_FUNC(exp2),
  TYPE_CHECK_FUNC(log2),
  TYPE_CHECK_FUNC(fract),
  {"", nullptr},
};
#undef TYPE_CHECK_FUNC
