#include "HLSLWriter.h"
#include "ProgramOption.h"
#include "write_helpers.h"
#include "Memory/MemoryRegion.h"
#include "Platform/Windows/WindowsHeader.h"
#include "Algorithm/Hasher.h"
#include <assert.h>

struct ConstantRemap {
  ConstantType constant_type;
  D3D_SHADER_VARIABLE_CLASS paramClass;
  D3D_SHADER_VARIABLE_TYPE paramType;
  u8 columns;
  u8 rows;
};

static const ConstantRemap s_constant_remaps[] = {
  {CONSTANT_BOOL, D3D_SVC_SCALAR, D3D_SVT_BOOL, 0, 0},
  {CONSTANT_INT, D3D_SVC_SCALAR, D3D_SVT_INT, 0, 0},
  {CONSTANT_FLOAT, D3D_SVC_SCALAR, D3D_SVT_FLOAT, 0, 0},
  {CONSTANT_VEC4, D3D_SVC_VECTOR, D3D_SVT_FLOAT, 0, 0},
  {CONSTANT_MAT3, D3D_SVC_MATRIX_COLUMNS, D3D_SVT_FLOAT, 3, 3},
  {CONSTANT_MAT4, D3D_SVC_MATRIX_COLUMNS, D3D_SVT_FLOAT, 4, 4},
  {CONSTANT_INT, D3D_SVC_OBJECT, D3D_SVT_SAMPLER, 0, 0},
  {CONSTANT_INT, D3D_SVC_OBJECT, D3D_SVT_SAMPLER2D, 0, 0},
  {CONSTANT_INT, D3D_SVC_OBJECT, D3D_SVT_SAMPLER3D, 0, 0},
  {CONSTANT_INT, D3D_SVC_OBJECT, D3D_SVT_SAMPLERCUBE, 0, 0},
};

ConstantType find_constant_type(const D3D11_SHADER_TYPE_DESC& constDesc) {
  for (u32 ii = 0; ii < ARRAY_SIZE(s_constant_remaps); ++ii) {
    const ConstantRemap& remap = s_constant_remaps[ii];

    if (remap.paramClass == constDesc.Class && remap.paramType == constDesc.Type) {
      if (D3D_SVC_MATRIX_COLUMNS != constDesc.Class) return remap.constant_type;
      if (remap.columns == constDesc.Columns && remap.rows == constDesc.Rows) return remap.constant_type;
    }
  }

  return CONSTANT_COUNT;
}

VarDeclNode* find_constant(ShaderNode* node, stringid name) {
  for (auto c : node->uniforms) {
    if (c->var_name.text.GetID() == name) return c;
  }
  return nullptr;
}

HLSLWriter::HLSLWriter(): _f(nullptr) {}
HLSLWriter::~HLSLWriter() {}

void HLSLWriter::SetOutput(const String& filename) {
  _filename = filename;
  _f = fopen(filename.GetCString(), "wb");
  assert(_f);
}

void HLSLWriter::WriteShader(ShaderNode* node) {
  _shader = node;
  if (_shader->shader_type == FRAGMENT_SHADER|| !node->inputs.empty()) {
    WriteString("struct ");
    WriteString(GetInputStructName());
    WriteString(" {\n");
    ++_indent;
    if (_shader->shader_type == FRAGMENT_SHADER) {
      WriteIndent();
      WriteString("float4 position : SV_POSITION;\n");
    }
    for (auto var_decl : node->inputs) WriteInputDecl(var_decl);
    --_indent;
    WriteString("};\n");
  }
  if (_shader->shader_type == VERTEX_SHADER || !node->outputs.empty()) {
    WriteString("struct ");
    WriteString(GetOutputStructName());
    WriteString(" {\n");
    ++_indent;
    if (_shader->shader_type == VERTEX_SHADER) {
      WriteIndent();
      WriteString("float4 position : SV_POSITION;\n");
    }
    for (int i = 0, n = node->outputs.size(); i < n; ++i) WriteOutputDecl(node->outputs[i], i);
    --_indent;
    WriteString("};\n");
  }
  if (!node->uniforms.empty()) {
    for (auto var_decl : node->uniforms) WriteUniformDecl(var_decl);
    WriteNewline();
  }
  if (!node->var_decls.empty()) {
    for (auto var_decl : node->var_decls) WriteVarDecl(var_decl);
    WriteNewline();
  }
  WriteBuiltinFunctions();
  if (!node->func_decls.empty()) {
    for (auto func_decl : node->func_decls) {
      WriteFuncDecl(func_decl);
      WriteNewline();
    }
  }
  fclose(_f);

  if (g_options.output_binary) {
    FILE* f = fopen(_filename.GetCString(), "rb");
    if (!f) exit(-1);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    auto source_mem = mem_alloc(size);
    fread(source_mem->data, source_mem->size, 1, f);
    fclose(f);
    UINT flags = D3DCOMPILE_DEBUG | D3DCOMPILE_OPTIMIZATION_LEVEL3 |
      D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
    const char* target = _shader->shader_type == VERTEX_SHADER ? "vs_5_0" : "ps_5_0";
    ID3DBlob* code;
    ID3DBlob* error_message;
    HRESULT hr = D3DCompile(source_mem->data, source_mem->size, _filename.GetCString(),
                            NULL, NULL, "main", target, flags, 0, &code, &error_message);
    if (FAILED(hr)) {
      printf("Compile shader %s failed.\n  %s\n", _filename.GetCString(),
             (const char*)error_message->GetBufferPointer());
      mem_free(source_mem);
      exit(-1);
    }
    mem_free(source_mem);

    // refection
    ID3D11ShaderReflection* reflect = NULL;
    D3DReflect(code->GetBufferPointer(), code->GetBufferSize(), __uuidof(ID3D11ShaderReflection), (void**)&reflect);
    D3D11_SHADER_DESC desc;
    reflect->GetDesc(&desc);

    // get cbuffer size and constant locations
    ID3D11ShaderReflectionConstantBuffer* cbuffer = reflect->GetConstantBufferByIndex(0);
    D3D11_SHADER_BUFFER_DESC buffer_desc;
    hr = cbuffer->GetDesc(&buffer_desc);
    if (FAILED(hr)) _shader->cbuffer_size = 0;
    else {
      _shader->cbuffer_size = (u16)buffer_desc.Size;

      for (auto& var_decl : _shader->uniforms) var_decl->loc = 0;

      for (u32 jj = 0; jj < buffer_desc.Variables; ++jj) {
        ID3D11ShaderReflectionVariable* var = cbuffer->GetVariableByIndex(jj);
        ID3D11ShaderReflectionType* type = var->GetType();
        D3D11_SHADER_VARIABLE_DESC var_desc;
        hr = var->GetDesc(&var_desc);
        if (FAILED(hr)) continue;
        D3D11_SHADER_TYPE_DESC const_desc;
        hr = type->GetDesc(&const_desc);
        if (FAILED(hr)) continue;
        ConstantType constant_type = find_constant_type(const_desc);

        if (CONSTANT_COUNT != constant_type && (var_desc.uFlags & D3D_SVF_USED)) {
          VarDeclNode* c = find_constant(_shader, hash_string(var_desc.Name));
          if (c) c->loc = (u16)var_desc.StartOffset;
        }
      }
    }

    f = fopen(g_options.output_filename.GetCString(), "wb");
    write_binary(_shader, code->GetBufferPointer(), code->GetBufferSize(), f);
    fclose(f);
  }
}

void HLSLWriter::WriteVersion() {}

void HLSLWriter::WriteInputDecl(VarDeclNode* node) {
  WriteIndent();
  WriteVarDecl(node, false);
  String semantic_name;
  if (_shader->shader_type == VERTEX_SHADER) semantic_name = node->var_name.text.Substr(2).ToUpper();
  else semantic_name = node->var_name.text.MakeUpper();
  WriteString(" : ");
  WriteString(semantic_name);
  WriteString(";\n");
}

void HLSLWriter::WriteOutputDecl(VarDeclNode* node, int index) {
  static const char* index_names[8] = {"0", "1", "2", "3", "4", "5", "6", "7"};
  WriteIndent();
  WriteVarDecl(node, false);
  String semantic_name;
  if (_shader->shader_type == FRAGMENT_SHADER) {
    semantic_name = "SV_TARGET";
    semantic_name.Append(index_names[index]);
  } else semantic_name = node->var_name.text.MakeUpper();
  WriteString(" : ");
  WriteString(semantic_name);
  WriteString(";\n");
}

void HLSLWriter::WriteUniformDecl(VarDeclNode* node) {
  WriteVarDecl(node);
  if (node->type_decl->type == VAR_TYPE_SAMPLER_2D) {
    WriteIndent();
    WriteString("SamplerState ");
    WriteString(node->var_name.text);
    WriteString("_sampler;\n");
  }
}

void HLSLWriter::WriteVarDecl(VarDeclNode* node, bool write_semicolon) {
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

void HLSLWriter::WriteFuncDecl(FuncDeclNode* node) {
  WriteTypeDecl(node->ret_type);
  WriteSpace();
  WriteString(node->func_name.text);
  WriteString("(");
  if (node->func_name.text == "main") {
    if (_shader->shader_type == FRAGMENT_SHADER || !_shader->inputs.empty()) {
      WriteString(GetInputStructName());
      WriteString(" IN, ");
    }
    if (_shader->shader_type == VERTEX_SHADER || !_shader->outputs.empty()) {
      WriteString("out ");
      WriteString(GetOutputStructName());
      WriteString(" OUT");
    }
  } else {
    for (int i = 0; i < node->num_params; ++i) {
      if (i > 0) WriteString(", ");
      if (node->params[i].flags == FUNC_PARAM_INOUT) WriteString("inout ");
      if (node->params[i].flags == FUNC_PARAM_OUT) WriteString("out ");
      WriteVarDecl(node->params[i].var_decl, false);
    }
  }
  WriteString(") {\n");
  ++_indent;
  WriteStatements(node->statements);
  --_indent;
  WriteString("}\n");
}

void HLSLWriter::WriteVarRef(VarRefNode* node) {
  if (node->ref_type == VAR_REF_TYPE_ID) WriteString(GetVarName(node->var_name->text));
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

void HLSLWriter::WriteAssignStatement(AssignStatementNode* node, bool write_semicolon) {
  if (write_semicolon) WriteIndent();
  if (node->left) WriteVarRef(node->left);
  WriteOp(node->assign_type);
  WriteExpression(node->right);
  if (write_semicolon) WriteString(";\n");
}

void HLSLWriter::WriteIfStatement(IfStatementNode* node) {
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

void HLSLWriter::WriteForStatement(ForStatementNode* node) {
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

void HLSLWriter::WriteTypeDecl(TypeDeclNode* node, bool write_num) {
  if (node->is_const) WriteString("static const ");
  WriteVarType(node->type);
  if (write_num && node->num > 1) fprintf(_f, "[%d]", node->num);
}

void HLSLWriter::WriteVarType(VarType type) {
  if (type == VAR_TYPE_BOOL) WriteString("bool");
  else if (type == VAR_TYPE_INT) WriteString("int");
  else if (type == VAR_TYPE_FLOAT) WriteString("float");
  else if (type == VAR_TYPE_VOID) WriteString("void");
  else if (type == VAR_TYPE_VEC2) WriteString("float2");
  else if (type == VAR_TYPE_VEC3) WriteString("float3");
  else if (type == VAR_TYPE_VEC4) WriteString("float4");
  else if (type == VAR_TYPE_IVEC2) WriteString("int2");
  else if (type == VAR_TYPE_IVEC3) WriteString("int3");
  else if (type == VAR_TYPE_IVEC4) WriteString("int4");
  else if (type == VAR_TYPE_MAT2) WriteString("float2x2");
  else if (type == VAR_TYPE_MAT3) WriteString("float3x3");
  else if (type == VAR_TYPE_MAT4) WriteString("float4x4");
  else if (type == VAR_TYPE_SAMPLER_2D) WriteString("Texture2D");
}

void HLSLWriter::WriteExpression(ExpressionNode* node) {
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
    if (node->type_cast.target_type->num > 1) {
      WriteString("{");
      for (int i = 0; i < node->type_cast.argc; ++i) {
        if (i > 0) WriteString(", ");
        WriteExpression(node->type_cast.argv[i]);
      }
      WriteString("}");
    } else if (node->type_cast.argc == 1) {
      WriteString("(");
      WriteTypeDecl(node->type_cast.target_type, true);
      WriteString(")(");
      WriteExpression(node->type_cast.argv[0]);
      WriteString(")");
    } else {
      WriteTypeDecl(node->type_cast.target_type, true);
      WriteString("(");
      for (int i = 0; i < node->type_cast.argc; ++i) {
        if (i > 0) WriteString(", ");
        WriteExpression(node->type_cast.argv[i]);
      }
      WriteString(")");
    }
  } else if (node->expr_type == EXPR_TYPE_FUNC_CALL) {
    auto func_name = GetFuncName(node->func_call.func_name->text, node->func_call.argc);
    if (IsSamplerFuncName(node->func_call.func_name->text)) {
      auto texture_name = node->func_call.argv[0]->var_ref->var_name->text;
      auto sampler_name = texture_name + "_sampler";
      WriteExpression(node->func_call.argv[0]);
      WriteString(".");
      WriteString(func_name);
      WriteString("(");
      WriteString(sampler_name);
      for (int i = 1; i < node->func_call.argc; ++i) {
        WriteString(", ");
        WriteExpression(node->func_call.argv[i]);
      }
      WriteString(")");
    } else {
      WriteString(func_name);
      WriteString("(");
      for (int i = 0; i < node->func_call.argc; ++i) {
        if (i > 0) WriteString(", ");
        WriteExpression(node->func_call.argv[i]);
      }
      WriteString(")");
    }
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
  } else if (node->expr_type == EXPR_TYPE_VEC_MUL) {
    WriteString("mul(");
    WriteExpression(node->first);
    WriteString(", ");
    WriteExpression(node->second);
    WriteString(")");
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

void HLSLWriter::WriteOp(ExpressionType type) {
  switch (type) {
  case EXPR_TYPE_ADD:
    WriteString(" + ");
    break;
  case EXPR_TYPE_MINUS:
    WriteString(" - ");
    break;
  case EXPR_TYPE_MUL:
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

void HLSLWriter::WriteStatements(vector<ASTNode*> nodes) {
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

void HLSLWriter::WriteReturnStatement(ReturnStatementNode* node) {
  WriteIndent();
  WriteString("return");
  if (node->ret_value) {
    WriteSpace();
    WriteExpression(node->ret_value);
  }
  WriteString(";");
  WriteNewline();
}

void HLSLWriter::WriteBreakStatement(BreakStatementNode* node) {
  WriteIndent();
  WriteString("break;\n");
}

void HLSLWriter::WriteContinueStatement(ContinueStatementNode* node) {
  WriteIndent();
  WriteString("continue;\n");
}

void HLSLWriter::WriteIndent() {
  for (int i = 0; i < _indent; ++i) fputs("  ", _f);
}

void HLSLWriter::WriteSpace() {
  fputc(' ', _f);
}

void HLSLWriter::WriteNewline() {
  fputc('\n', _f);
}

void HLSLWriter::WriteString(const String& s) {
  fputs(s.GetCString(), _f);
}

String HLSLWriter::GetVarName(const String& var_name) {
  static const String vs_pos = "OUT.position";
  static const String ps_pos = "IN.position";
  if (var_name == "POSITION") return _shader->shader_type == VERTEX_SHADER ? vs_pos : ps_pos;
  else {
    bool is_input = false;
    for (auto var_decl : _shader->inputs) {
      if (var_decl->var_name.text == var_name) {
        is_input = true;
        break;
      }
    }
    if (is_input) return "IN." + var_name;

    bool is_output = false;
    for (auto var_decl : _shader->outputs) {
      if (var_decl->var_name.text == var_name) {
        is_output = true;
        break;
      }
    }
    if (is_output) return "OUT." + var_name;
    return var_name;
  }
}

String HLSLWriter::GetFuncName(const String& func_name, int argc) {
  if (func_name == "mix")  return "lerp";
  else if (func_name == "texture")  return argc == 2 ? "Sample" : "SampleBias";
  else if (func_name == "texelFetch")  return "__texelFetch";
  else if (func_name == "textureSize")  return "__textureSize";
  return func_name;
}

void HLSLWriter::GetPrecedence(ExpressionType type, int& out_pred, bool& out_right_assoc) {
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

void HLSLWriter::WriteBuiltinFunctions() {
  WriteString("int2 __textureSize(Texture2D textureObj, int dim) {\n"
              "  uint width;\n"
              "  uint height;\n"
              "  textureObj.GetDimensions(width, height);\n"
              "  return int2(width, height)\n;"
              "}\n");
  WriteString("float4 __texelFetch(Texture2D textureObj, int2 p, int lod) {\n"
              "  return textureObj.Load(int3(p.x, p.y, lod));\n"
              "}\n");
}

bool HLSLWriter::IsSamplerFuncName(const String& func_name) {
  return func_name == "texture";
}

const String& HLSLWriter::GetInputStructName() const {
  static String vs_input = "VS_INPUT";
  static String ps_input = "PS_INPUT";
  return _shader->shader_type == VERTEX_SHADER ? vs_input : ps_input;
}

const String& HLSLWriter::GetOutputStructName() const {
  static String vs_output = "VS_OUTPUT";
  static String ps_output = "PS_OUTPUT";
  return _shader->shader_type == VERTEX_SHADER ? vs_output : ps_output;
}
