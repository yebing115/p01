#include "C3PCH.h"
#include "MaterialLoader.h"
#include "Graphics/Material/Material.h"

static ShaderHandle load_bare_shader(const char* filename, ShaderInfo::Header* header = nullptr) {
  auto file = FileSystem::Instance()->OpenRead(filename);
  if (file) {
    auto mem = mem_alloc(file->GetSize());
    file->ReadBytes(mem->data, mem->size);
    FileSystem::Instance()->Close(file);
    return GraphicsRenderer::Instance()->CreateShader(mem, header);
  }
  return ShaderHandle();
}

static bool compute_shader_binary_filename(JsonReader& reader, SubShader* _shader,
                                           const char* filename_key, const char* defines_key,
                                           char* binary_filename, u32 max_size) {
  char filename[MAX_ASSET_NAME];
  char defines[1024] = "";
  char p[MAX_MATERIAL_KEY_LEN];
  int num_defines = 0;
  if (reader.ReadString(filename_key, filename, MAX_ASSET_NAME)) {
    char* suffix = strrchr(filename, '.');
    c3_assert(suffix);
    *suffix++ = 0;
    if (reader.BeginReadArray(defines_key)) {
      while (reader.ReadStringElement(p, MAX_MATERIAL_KEY_LEN)) {
        if (num_defines > 0) strcat(defines, ";");
        strncat(defines, p, sizeof(p));
        ++num_defines;
      }
      reader.EndReadArray();
    }

    stringid defines_id = String::GetID(defines);
    snprintf(binary_filename, max_size, "Shaders/%s/%s/%s_%08x.%sb",
             _shader->_technique, _shader->_pass, filename, defines_id, suffix);
    return true;
  }
  return false;
}

static Asset* load_texture_asset(const char* model_filename, const char* filename) {
  String path(MAX_ASSET_NAME);
  if (strchr(filename, '.')) {
    path.Set(model_filename);
    path.RemoveLastSection();
    path.Append('/');
    path.Append(filename);
  } else path.Set(filename);
  return AssetManager::Instance()->Load(ASSET_TYPE_TEXTURE, path.GetCString());
}

static bool load_material_shader_param(const char* asset_filename, JsonReader& reader,
                                       MaterialParam& param) {
  auto GR = GraphicsRenderer::Instance();
  char type_str[MAX_MATERIAL_KEY_LEN] = "";
  char value_str[MAX_MATERIAL_KEY_LEN] = "";
  if (reader.BeginReadObject()) {
    if (reader.ReadString("type", type_str, sizeof(type_str))) {
      if (strcmp(type_str, "float") == 0) param._type = MATERIAL_PARAM_FLOAT;
      else if (strcmp(type_str, "vec2") == 0) param._type = MATERIAL_PARAM_VEC2;
      else if (strcmp(type_str, "vec3") == 0) param._type = MATERIAL_PARAM_VEC3;
      else if (strcmp(type_str, "vec4") == 0) param._type = MATERIAL_PARAM_VEC4;
      else if (strcmp(type_str, "sampler2D") == 0) param._type = MATERIAL_PARAM_TEXTURE2D;
      else {
        c3_log("Failed to parse material param type '%s'.\n", type_str);
        reader.EndReadObject();
        return false;
      }
      if (param._type == MATERIAL_PARAM_FLOAT) {
        param._constant_handle = GR->CreateConstant(String::GetID(param._name), CONSTANT_FLOAT);
        reader.ReadFloat("value", param._vec[0]);
      } else if (param._type == MATERIAL_PARAM_VEC2) {
        param._constant_handle = GR->CreateConstant(String::GetID(param._name), CONSTANT_VEC2);
        if (reader.BeginReadArray("value")) {
          reader.ReadFloatElement(param._vec[0]);
          reader.ReadFloatElement(param._vec[1]);
          reader.EndReadArray();
        }
      } else if (param._type == MATERIAL_PARAM_VEC3) {
        param._constant_handle = GR->CreateConstant(String::GetID(param._name), CONSTANT_VEC3);
        if (reader.ReadString("value", value_str, sizeof(value_str))) {
          if (strcmp(value_str, "WHITE") == 0) {
            param._vec[0] = 1.f;
            param._vec[1] = 1.f;
            param._vec[2] = 1.f;
          } else if (strcmp(value_str, "RED") == 0) {
            param._vec[0] = 1.f;
            param._vec[1] = 0.f;
            param._vec[2] = 0.f;
          } else if (strcmp(value_str, "GREEN") == 0) {
            param._vec[0] = 0.f;
            param._vec[1] = 1.f;
            param._vec[2] = 0.f;
          } else if (strcmp(value_str, "BLUE") == 0) {
            param._vec[0] = 0.f;
            param._vec[1] = 0.f;
            param._vec[2] = 1.f;
          } else if (strcmp(value_str, "BLACK") == 0) {
            param._vec[0] = 0.f;
            param._vec[1] = 0.f;
            param._vec[2] = 0.f;
          } else if (strcmp(value_str, "UP") == 0) {
            param._vec[0] = 0.5f;
            param._vec[1] = 0.5f;
            param._vec[2] = 1.f;
          } else {
            c3_log("Failed to parse constant vec3 value '%s'\n", value_str);
            param._vec[0] = 0.f;
            param._vec[1] = 0.f;
            param._vec[2] = 0.f;
          }
        } else if (reader.BeginReadArray("value")) {
          reader.ReadFloatElement(param._vec[0]);
          reader.ReadFloatElement(param._vec[1]);
          reader.ReadFloatElement(param._vec[2]);
          reader.EndReadArray();
        }
      } else if (param._type == MATERIAL_PARAM_VEC4) {
        param._constant_handle = GR->CreateConstant(String::GetID(param._name), CONSTANT_VEC4);
        if (reader.ReadString("value", value_str, sizeof(value_str))) {
          if (strcmp(value_str, "WHITE") == 0) {
            param._vec[0] = 1.f;
            param._vec[1] = 1.f;
            param._vec[2] = 1.f;
            param._vec[3] = 1.f;
          } else if (strcmp(value_str, "RED") == 0) {
            param._vec[0] = 1.f;
            param._vec[1] = 0.f;
            param._vec[2] = 0.f;
            param._vec[3] = 1.f;
          } else if (strcmp(value_str, "GREEN") == 0) {
            param._vec[0] = 0.f;
            param._vec[1] = 1.f;
            param._vec[2] = 0.f;
            param._vec[3] = 1.f;
          } else if (strcmp(value_str, "BLUE") == 0) {
            param._vec[0] = 0.f;
            param._vec[1] = 0.f;
            param._vec[2] = 1.f;
            param._vec[3] = 1.f;
          } else if (strcmp(value_str, "BLACK") == 0) {
            param._vec[0] = 0.f;
            param._vec[1] = 0.f;
            param._vec[2] = 0.f;
            param._vec[3] = 1.f;
          } else if (strcmp(value_str, "UP") == 0) {
            param._vec[0] = 0.5f;
            param._vec[1] = 0.5f;
            param._vec[2] = 1.f;
            param._vec[3] = 0.f;
          } else {
            c3_log("Failed to parse constant vec4 value '%s'\n", value_str);
            param._vec[0] = 0.f;
            param._vec[1] = 0.f;
            param._vec[2] = 0.f;
            param._vec[3] = 0.f;
          }
        } else if (reader.BeginReadArray("value")) {
          reader.ReadFloatElement(param._vec[0]);
          reader.ReadFloatElement(param._vec[1]);
          reader.ReadFloatElement(param._vec[2]);
          reader.ReadFloatElement(param._vec[3]);
          reader.EndReadArray();
        }
      } else if (param._type == MATERIAL_PARAM_TEXTURE2D) {
        param._constant_handle = GR->CreateConstant(String::GetID(param._name), CONSTANT_INT);
        param._tex2d._unit = UINT8_MAX;
        param._tex2d._flags = C3_TEXTURE_MAG_ANISOTROPIC | C3_TEXTURE_MIN_ANISOTROPIC;
        char texture_filename[MAX_ASSET_NAME];
        char flag_str[MAX_MATERIAL_KEY_LEN];
        if (reader.ReadString("value", texture_filename, sizeof(texture_filename))) {
          param._tex2d._asset = load_texture_asset(asset_filename, texture_filename);
        }
        if (reader.ReadString("flags", flag_str, sizeof(flag_str))) {
          if (strcmp(flag_str, "UV_CLAMP") == 0) param._tex2d._flags |= C3_TEXTURE_U_CLAMP | C3_TEXTURE_V_CLAMP;
          else if (strcmp(flag_str, "UV_MIRROR") == 0) param._tex2d._flags |= C3_TEXTURE_U_MIRROR | C3_TEXTURE_V_MIRROR;
        }
      }
    }
    reader.EndReadObject();
    return true;
  }
  return false;
}

static bool load_material_param(const char* asset_filename, JsonReader& reader,
                                MaterialParam& param) {
  char type_str[MAX_MATERIAL_KEY_LEN] = "";
  char value_str[MAX_MATERIAL_KEY_LEN] = "";
  if (reader.BeginReadObject()) {
    if (param._type == MATERIAL_PARAM_FLOAT) {
      reader.ReadFloat("value", param._vec[0]);
    } else if (param._type == MATERIAL_PARAM_VEC2) {
      if (reader.BeginReadArray("value")) {
        reader.ReadFloatElement(param._vec[0]);
        reader.ReadFloatElement(param._vec[1]);
        reader.EndReadArray();
      }
    } else if (param._type == MATERIAL_PARAM_VEC3) {
      if (reader.ReadString("value", value_str, sizeof(value_str))) {
        if (strcmp(value_str, "WHITE") == 0) {
          param._vec[0] = 1.f;
          param._vec[1] = 1.f;
          param._vec[2] = 1.f;
        } else if (strcmp(value_str, "RED") == 0) {
          param._vec[0] = 1.f;
          param._vec[1] = 0.f;
          param._vec[2] = 0.f;
        } else if (strcmp(value_str, "GREEN") == 0) {
          param._vec[0] = 0.f;
          param._vec[1] = 1.f;
          param._vec[2] = 0.f;
        } else if (strcmp(value_str, "BLUE") == 0) {
          param._vec[0] = 0.f;
          param._vec[1] = 0.f;
          param._vec[2] = 1.f;
        } else if (strcmp(value_str, "BLACK") == 0) {
          param._vec[0] = 0.f;
          param._vec[1] = 0.f;
          param._vec[2] = 0.f;
        } else if (strcmp(value_str, "UP") == 0) {
          param._vec[0] = 0.5f;
          param._vec[1] = 0.5f;
          param._vec[2] = 1.f;
        } else {
          c3_log("Failed to parse constant vec3 value '%s'\n", value_str);
          param._vec[0] = 0.f;
          param._vec[1] = 0.f;
          param._vec[2] = 0.f;
        }
      } else if (reader.BeginReadArray("value")) {
        reader.ReadFloatElement(param._vec[0]);
        reader.ReadFloatElement(param._vec[1]);
        reader.ReadFloatElement(param._vec[2]);
        reader.EndReadArray();
      }
    } else if (param._type == MATERIAL_PARAM_VEC4) {
      if (reader.ReadString("value", value_str, sizeof(value_str))) {
        if (strcmp(value_str, "WHITE") == 0) {
          param._vec[0] = 1.f;
          param._vec[1] = 1.f;
          param._vec[2] = 1.f;
          param._vec[3] = 1.f;
        } else if (strcmp(value_str, "RED") == 0) {
          param._vec[0] = 1.f;
          param._vec[1] = 0.f;
          param._vec[2] = 0.f;
          param._vec[3] = 1.f;
        } else if (strcmp(value_str, "GREEN") == 0) {
          param._vec[0] = 0.f;
          param._vec[1] = 1.f;
          param._vec[2] = 0.f;
          param._vec[3] = 1.f;
        } else if (strcmp(value_str, "BLUE") == 0) {
          param._vec[0] = 0.f;
          param._vec[1] = 0.f;
          param._vec[2] = 1.f;
          param._vec[3] = 1.f;
        } else if (strcmp(value_str, "BLACK") == 0) {
          param._vec[0] = 0.f;
          param._vec[1] = 0.f;
          param._vec[2] = 0.f;
          param._vec[3] = 1.f;
        } else if (strcmp(value_str, "UP") == 0) {
          param._vec[0] = 0.5f;
          param._vec[1] = 0.5f;
          param._vec[2] = 1.f;
          param._vec[3] = 0.f;
        } else {
          c3_log("Failed to parse constant vec4 value '%s'\n", value_str);
          param._vec[0] = 0.f;
          param._vec[1] = 0.f;
          param._vec[2] = 0.f;
          param._vec[3] = 0.f;
        }
      } else if (reader.BeginReadArray("value")) {
        reader.ReadFloatElement(param._vec[0]);
        reader.ReadFloatElement(param._vec[1]);
        reader.ReadFloatElement(param._vec[2]);
        reader.ReadFloatElement(param._vec[3]);
        reader.EndReadArray();
      }
    } else if (param._type == MATERIAL_PARAM_TEXTURE2D) {
      param._tex2d._flags = C3_TEXTURE_NONE;
      char texture_filename[MAX_ASSET_NAME];
      char flag_str[MAX_MATERIAL_KEY_LEN];
      if (reader.ReadString("value", texture_filename, sizeof(texture_filename))) {
        param._tex2d._asset = load_texture_asset(asset_filename, texture_filename);
      }
      if (reader.ReadString("flags", flag_str, sizeof(flag_str))) {
        if (strcmp(flag_str, "UV_CLAMP") == 0) param._tex2d._flags = C3_TEXTURE_U_CLAMP | C3_TEXTURE_V_CLAMP;
        else if (strcmp(flag_str, "UV_MIRROR") == 0) param._tex2d._flags = C3_TEXTURE_U_MIRROR | C3_TEXTURE_V_MIRROR;
      }
    }
    reader.EndReadObject();
    return true;
  }
  return false;
}

DEFINE_JOB_ENTRY(load_material_shader) {
  auto asset = (Asset*)arg;
  auto f = FileSystem::Instance()->OpenRead(asset->_desc._filename);
  if (!f) {
    asset->_state = ASSET_STATE_EMPTY;
    return;
  }
  auto mem = mem_alloc(f->GetSize() + 1);
  f->ReadBytes(mem->data, f->GetSize());
  *((u8*)mem->data + f->GetSize()) = 0;
  FileSystem::Instance()->Close(f);
  
  SpinLockGuard lock_guard(&asset->_lock);
  u16 num_textures = 0;
  char shader_binary_filename[MAX_ASSET_NAME];
  MaterialShader material_shader;
  vector<AssetDesc> texture_descs;
  JsonReader reader((const char*)mem->data);
  c3_assert(reader.IsValid());
  
  material_shader._num_sub_shaders = 0;
  while (reader.BeginReadObject()) {
    SubShader* sub_shader = material_shader._sub_shaders + material_shader._num_sub_shaders;
    reader.ReadString("technique", sub_shader->_technique, sizeof(sub_shader->_technique));
    reader.ReadString("pass", sub_shader->_pass, sizeof(sub_shader->_pass));

    ShaderHandle vsh, fsh;
    ShaderInfo::Header vs_header, fs_header;
    if (compute_shader_binary_filename(reader, sub_shader, "vs_source", "vs_defines",
                                       shader_binary_filename, sizeof(shader_binary_filename))) {
      vsh = load_bare_shader(shader_binary_filename, &vs_header);
    }
    if (compute_shader_binary_filename(reader, sub_shader, "fs_source", "fs_defines",
                                       shader_binary_filename, sizeof(shader_binary_filename))) {
      fsh = load_bare_shader(shader_binary_filename, &fs_header);
    }
    sub_shader->_program = GraphicsRenderer::Instance()->CreateProgram(vsh, fsh);

    reader.BeginReadObject("properties");
    char mat_key[MAX_MATERIAL_KEY_LEN];
    JsonValueType value_type;
    sub_shader->_num_params = 0;
    while (reader.Peek(mat_key, sizeof(mat_key), &value_type)) {
      c3_assert(value_type == JSON_VALUE_OBJECT);
      MaterialParam* param = sub_shader->_params + sub_shader->_num_params;
      strncpy(param->_name, mat_key, sizeof(mat_key));
      if (load_material_shader_param(asset->_desc._filename, reader, *param)) {
        if (param->_type == MATERIAL_PARAM_TEXTURE2D) {
          ++num_textures;
          texture_descs.push_back(param->_tex2d._asset->_desc);
          stringid name_id = String::GetID(param->_name);
          for (u8 i = 0; i < fs_header.num_constants; ++i) {
            if (fs_header.constants[i].name == name_id) {
              param->_tex2d._unit = fs_header.constants[i].loc;
            }
          }
        }
        ++sub_shader->_num_params;
      }
    }
    reader.EndReadObject(); // "properties"
    reader.EndReadObject();
    ++material_shader._num_sub_shaders;
  }

  mem_free(mem); // json data

  u32 asset_memory_size = ASSET_MEMORY_SIZE(num_textures, sizeof(MaterialShader));
  asset->_header = (AssetMemoryHeader*)C3_ALLOC(g_allocator, asset_memory_size);
  asset->_header->_size = asset_memory_size;
  asset->_header->_num_depends = num_textures;
  if (num_textures > 0) memcpy(asset->_header->_depends, texture_descs.data(),
                               sizeof(AssetDesc) * num_textures);
  auto ms = (MaterialShader*)asset->_header->GetData();
  memcpy(ms, &material_shader, sizeof(MaterialShader));
  asset->_state = ASSET_STATE_READY;
}

static MaterialParam* find_material_param(MaterialShader* shader, const char* name) {
  for (u32 i = 0; i < shader->_num_sub_shaders; ++i) {
    for (u32 j = 0; j < shader->_sub_shaders[i]._num_params; ++j) {
      if (strcmp(shader->_sub_shaders[i]._params[j]._name, name) == 0) return shader->_sub_shaders[i]._params + j;
    }
  }
  return nullptr;
}

DEFINE_JOB_ENTRY(load_material) {
  auto asset = (Asset*)arg;
  auto f = FileSystem::Instance()->OpenRead(asset->_desc._filename);
  if (!f) {
    asset->_state = ASSET_STATE_EMPTY;
    return;
  }
  auto mem = mem_alloc(f->GetSize() + 1);
  f->ReadBytes(mem->data, f->GetSize());
  *((u8*)mem->data + f->GetSize()) = 0;
  FileSystem::Instance()->Close(f);

  JsonReader reader((const char*)mem->data);
  c3_assert(reader.IsValid());
  char shader_filename[MAX_ASSET_NAME] = "Shaders/";
  int n = strlen(shader_filename);
  reader.ReadString("shader", shader_filename + n, MAX_ASSET_NAME - n);
  strcat(shader_filename, ".mas");
  Asset* shader_asset = AssetManager::Instance()->Load(ASSET_TYPE_MATERIAL_SHADER, shader_filename);
  c3_assert(shader_asset);
  {
    SpinLockGuard shader_lock_guard(&shader_asset->_lock);
    if (shader_asset->_state == ASSET_STATE_READY) {
      auto shader = (MaterialShader*)shader_asset->_header->GetData();
      SpinLockGuard lock_guard(&asset->_lock);
      u32 asset_mem_size = ASSET_MEMORY_SIZE(1 + shader_asset->_header->_num_depends, sizeof(Material));
      asset->_header = (AssetMemoryHeader*)C3_ALLOC(g_allocator, asset_mem_size);
      asset->_header->_size = asset_mem_size;

      asset->_header->_num_depends = 1 + shader_asset->_header->_num_depends;
      auto& depend_desc = asset->_header->_depends[0];
      depend_desc._type = ASSET_TYPE_MATERIAL_SHADER;
      depend_desc._flags = 0;
      strcpy(depend_desc._filename, shader_filename);
      memcpy(asset->_header->_depends + 1, shader_asset->_header->_depends,
             shader_asset->_header->_num_depends * sizeof(AssetDesc));
      
      auto mat = (Material*)asset->_header->GetData();
      mat->_shader_asset = shader_asset;
      mat->_num_params = 0;
      
      reader.BeginReadObject("properties");
      JsonValueType value_type;
      char param_name[MAX_MATERIAL_KEY_LEN];
      int num_textures = 0;
      while (reader.Peek(param_name, sizeof(param_name), &value_type)) {
        c3_assert(value_type == JSON_VALUE_OBJECT);
        MaterialParam* shader_param = find_material_param(shader, param_name);
        if (!shader_param) {
          reader.BeginReadObject();
          reader.EndReadObject();
          continue;
        }
        MaterialParam* param = mat->_params + mat->_num_params;
        strcpy(param->_name, shader_param->_name);
        param->_constant_handle = shader_param->_constant_handle;
        param->_type = shader_param->_type;
        load_material_param(asset->_desc._filename, reader, *param);
        if (param->_type == MATERIAL_PARAM_TEXTURE2D) {
          asset->_header->_depends[1 + num_textures++] = param->_tex2d._asset->_desc;
        }
        ++mat->_num_params;
      }
      reader.EndReadObject();
    }
  }
  mem_free(mem);
  asset->_state = ASSET_STATE_READY;
}

static atomic_int* material_shader_load_async(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_LOADING);
  auto JS = JobScheduler::Instance();
  Job job;
  job.InitWorkerJob(load_material_shader, asset);
  return JS->SubmitJobs(&job, 1);
}

static void material_shader_unload(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_UNLOADING);
}

static atomic_int* material_load_async(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_LOADING);
  auto JS = JobScheduler::Instance();
  Job job;
  job.InitWorkerJob(load_material, asset);
  return JS->SubmitJobs(&job, 1);
}

static void material_unload(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_UNLOADING);
}

AssetOperations MATERIAL_SHADER_OPS = {
  &material_shader_load_async,
  &material_shader_unload,
};

AssetOperations MATERIAL_OPS = {
  &material_load_async,
  &material_unload,
};
