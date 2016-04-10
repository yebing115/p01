#include "write_helpers.h"
#include "Graphics/GraphicsRenderer.h"

struct AttrToSemantic {
  stringid attr_name;
  stringid semantic;
};

AttrToSemantic s_attr_to_semantic[] = {
  {hash_string("a_position"), hash_string("POSITION")},
  {hash_string("a_normal"), hash_string("NORMAL")},
  {hash_string("a_tangent"), hash_string("TANGENT")},
  {hash_string("a_bitangent"), hash_string("BITANGENT")},
  {hash_string("a_texcoord0"), hash_string("TEXCOORD0")},
  {hash_string("a_texcoord1"), hash_string("TEXCOORD1")},
  {hash_string("a_color0"), hash_string("COLOR0")},
  {hash_string("a_color1"), hash_string("COLOR1")},
};

stringid get_semantic(stringid attr_name) {
  for (u32 i = 0; i < ARRAY_SIZE(s_attr_to_semantic); ++i) {
    if (s_attr_to_semantic[i].attr_name == attr_name) return s_attr_to_semantic[i].semantic;
  }
  return 0;
}

void write_binary(ShaderNode* shader, const void* payload, size_t playload_size, FILE* f) {
  ShaderInfo::Header header;
  unordered_set<String> names;
  u16 name_len = 0;
  memset(&header, 0, sizeof(header));
  header.magic = shader->shader_type == VERTEX_SHADER ? C3_CHUNK_MAGIC_VSH : C3_CHUNK_MAGIC_FSH;
  header.num_inputs = (u8)shader->inputs.size();
  for (int i = 0; i < header.num_inputs; ++i) {
    VarDeclNode* var_decl = shader->inputs[i];
    ShaderInfo::Input& input = header.inputs[i];
    name_len += (u16)var_decl->var_name.text.GetLength() + 1;
    names.insert(var_decl->var_name.text);
    input.name = var_decl->var_name.text.GetID();
    input.semantic = get_semantic(input.name);
    auto var_type = var_decl->type_decl->type;
    if (var_type == VAR_TYPE_INT) {
      input.data_type = INT8_TYPE;
      input.num = 1;
    } else if (var_type == VAR_TYPE_IVEC2 || var_type == VAR_TYPE_IVEC3 || var_type == VAR_TYPE_IVEC4) {
      input.data_type = INT8_TYPE;
      input.num = u16(var_type - VAR_TYPE_IVEC2 + 2);
    } else if (var_type == VAR_TYPE_FLOAT) {
      input.data_type = FLOAT_TYPE;
      input.num = 1;
    } else if (var_type == VAR_TYPE_VEC2 || var_type == VAR_TYPE_VEC3 || var_type == VAR_TYPE_VEC4) {
      input.data_type = FLOAT_TYPE;
      input.num = u16(var_type - VAR_TYPE_VEC2 + 2);
    } else {
      printf("Invalid shader input var_type: %s\n", VAR_TYPE_NAMES[var_type]);
      exit(-1);
    }
    input.normalized = 0;
    input.as_int = 0;
  }

  header.num_outputs = (u8)shader->outputs.size();
  for (int i = 0; i < header.num_outputs; ++i) {
    VarDeclNode* var_decl = shader->outputs[i];
    ShaderInfo::Output& output = header.outputs[i];
    name_len += (u16)var_decl->var_name.text.GetLength() + 1;
    names.insert(var_decl->var_name.text);
    output.name = var_decl->var_name.text.GetID();
    auto var_type = var_decl->type_decl->type;
    if (var_type == VAR_TYPE_INT) {
      output.data_type = INT8_TYPE;
      output.num = 1;
    } else if (var_type == VAR_TYPE_IVEC2 || var_type == VAR_TYPE_IVEC3 || var_type == VAR_TYPE_IVEC4) {
      output.data_type = INT8_TYPE;
      output.num = u8(var_type - VAR_TYPE_IVEC2 + 2);
    } else if (var_type == VAR_TYPE_FLOAT) {
      output.data_type = FLOAT_TYPE;
      output.num = 1;
    } else if (var_type == VAR_TYPE_VEC2 || var_type == VAR_TYPE_VEC3 || var_type == VAR_TYPE_VEC4) {
      output.data_type = FLOAT_TYPE;
      output.num = u8(var_type - VAR_TYPE_VEC2 + 2);
    } else {
      printf("Invalid shader output var_type: %s\n", VAR_TYPE_NAMES[var_type]);
      exit(-1);
    }
  }

  header.num_constants = (u8)shader->uniforms.size();
  for (int i = 0; i < header.num_constants; ++i) {
    VarDeclNode* var_decl = shader->uniforms[i];
    ShaderInfo::Constant& constant = header.constants[i];
    name_len += (u16)var_decl->var_name.text.GetLength() + 1;
    names.insert(var_decl->var_name.text);
    constant.name = var_decl->var_name.text.GetID();
    auto var_type = var_decl->type_decl->type;
    if (var_type == VAR_TYPE_INT || var_type == VAR_TYPE_SAMPLER_2D) constant.constant_type = CONSTANT_INT;
    else if (var_type == VAR_TYPE_FLOAT) constant.constant_type = CONSTANT_FLOAT;
    else if (var_type == VAR_TYPE_VEC2) constant.constant_type = CONSTANT_VEC2;
    else if (var_type == VAR_TYPE_VEC3) constant.constant_type = CONSTANT_VEC3;
    else if (var_type == VAR_TYPE_VEC4) constant.constant_type = CONSTANT_VEC4;
    else if (var_type == VAR_TYPE_MAT3) constant.constant_type = CONSTANT_MAT3;
    else if (var_type == VAR_TYPE_MAT4) constant.constant_type = CONSTANT_MAT4;
    else if (var_type == VAR_TYPE_BOOL) constant.constant_type = CONSTANT_BOOL32;
    else {
      printf("Invalid shader uniform var_type: %s\n", VAR_TYPE_NAMES[var_type]);
      exit(-1);
    }
    constant.num = (u8)var_decl->type_decl->num;
    constant.loc = var_decl->loc;
  }

  header.string_offset = ALIGN_16(sizeof(header));
  header.code_offset = ALIGN_16(header.string_offset + name_len);
  header.code_size = playload_size;
  header.cbuffer_size = shader->cbuffer_size;

  u8 zeros[16];
  memset(zeros, 0, sizeof(zeros));
  fseek(f, 0, SEEK_SET);
  fwrite(&header, sizeof(header), 1, f);
  int pad = header.string_offset - sizeof(header);
  fwrite(zeros, 1, pad, f);
  for (auto& name : names) fwrite(name.GetCString(), name.GetLength() + 1, 1, f);
  pad = header.code_offset - (header.string_offset + name_len);
  fwrite(zeros, 1, pad, f);
  fwrite(payload, playload_size, 1, f);
  fflush(f);
}
