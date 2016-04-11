#include "C3PCH.h"
#include "MaterialLoader.h"
#include "Graphics/Material/Material.h"

static ShaderHandle load_raw_shader(const char* tech, const char* pass, const char* filename) {
  return ShaderHandle();
}

static bool load_material_param(JsonReader& reader, MaterialParam& param, int& num_textures) {
  char type_str[MAX_MATERIAL_KEY_LEN] = "";
  if (reader.BeginReadObject()) {
    if (reader.ReadString("type", type_str, sizeof(type_str))) {
      if (strcmp(type_str, "float") == 0) param._type = MATERIAL_PARAM_FLOAT;
      else if (strcmp(type_str, "vec2") == 0) param._type = MATERIAL_PARAM_VEC2;
      else if (strcmp(type_str, "vec3") == 0) param._type = MATERIAL_PARAM_VEC2;
      else if (strcmp(type_str, "vec4") == 0) param._type = MATERIAL_PARAM_VEC2;
      else if (strcmp(type_str, "sampler2D") == 0) param._type = MATERIAL_PARAM_TEXTURE2D;
      else {
        c3_log("Failed to parse material param type '%s'.\n", type_str);
        reader.EndReadObject();
        return false;
      }
      if (param._type == MATERIAL_PARAM_FLOAT) reader.ReadFloat("value", param._vec[0]);
      else if (param._type == MATERIAL_PARAM_VEC2) {
        if (reader.BeginReadArray("value")) {
          reader.ReadFloatElement(param._vec[0]);
          reader.ReadFloatElement(param._vec[1]);
          reader.EndReadArray();
        }
      } else if (param._type == MATERIAL_PARAM_VEC3) {
        if (reader.BeginReadArray("value")) {
          reader.ReadFloatElement(param._vec[0]);
          reader.ReadFloatElement(param._vec[1]);
          reader.ReadFloatElement(param._vec[2]);
          reader.EndReadArray();
        }
      } else if (param._type == MATERIAL_PARAM_VEC4) {
        if (reader.BeginReadArray("value")) {
          reader.ReadFloatElement(param._vec[0]);
          reader.ReadFloatElement(param._vec[1]);
          reader.ReadFloatElement(param._vec[2]);
          reader.ReadFloatElement(param._vec[3]);
          reader.EndReadArray();
        }
      } else if (param._type == MATERIAL_PARAM_TEXTURE2D) {
        param._tex2d._unit = num_textures++;
        param._tex2d._flags = C3_TEXTURE_NONE;
        char asset_name[MAX_ASSET_NAME];
        char flag_str[MAX_MATERIAL_KEY_LEN];
        if (reader.ReadString("value", asset_name, sizeof(asset_name))) {
          param._asset = AssetManager::Instance()->Load(ASSET_TYPE_TEXTURE, asset_name);
        }
        if (reader.ReadString("flags", flag_str, sizeof(flag_str))) {
          if (strcmp(flag_str, "UV_CLAMP") == 0) param._tex2d._flags = C3_TEXTURE_U_CLAMP | C3_TEXTURE_V_CLAMP;
          else if (strcmp(flag_str, "UV_MIRROR") == 0) param._tex2d._flags = C3_TEXTURE_U_MIRROR | C3_TEXTURE_V_MIRROR;
        }
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
  char vs_source[MAX_ASSET_NAME];
  char fs_source[MAX_ASSET_NAME];
  MaterialShader _shader;
  vector<MaterialParam> _params;
  JsonReader reader((const char*)mem->data);
  c3_assert(reader.IsValid());

  reader.ReadString("technique", _shader._technique, sizeof(_shader._technique));
  reader.ReadString("pass", _shader._pass, sizeof(_shader._pass));
  
  reader.ReadString("vs_source", vs_source, sizeof(vs_source));
  reader.ReadString("fs_source", fs_source, sizeof(fs_source));
  //char shader_filename[MAX_ASSET_NAME];
  //snprintf(shader_filename, sizeof(shader_filename), "Shaders/%s/%s/%s", _shader.technique, _shader.pass, vs_source);
  auto vsh = load_raw_shader(_shader._technique, _shader._pass, vs_source);
  auto fsh = load_raw_shader(_shader._technique, _shader._pass, fs_source);
  _shader._program = GraphicsRenderer::Instance()->CreateProgram(vsh, fsh);

  //const char* base_name = max<const char*>(strrchr(asset->_desc._filename, '/'), strrchr(asset->_desc._filename, '\\'));
  //if (!base_name) base_name = asset->_desc._filename;

  reader.BeginReadObject("properties");
  char mat_key[MAX_MATERIAL_KEY_LEN];
  JsonValueType value_type;
  while (reader.Peek(mat_key, sizeof(mat_key), &value_type)) {
    c3_assert(value_type == JSON_VALUE_STRING);
    MaterialParam param;
    strncpy(param._name, mat_key, sizeof(param._name));
    if (load_material_param(reader, param, num_textures)) _params.push_back(param);
  }
  reader.EndReadObject();
  
  mem_free(mem);
  asset->_state = ASSET_STATE_READY;
}

DEFINE_JOB_ENTRY(load_material) {
  auto asset = (Asset*)arg;
  auto f = FileSystem::Instance()->OpenRead(asset->_desc._filename);
  if (!f) {
    asset->_state = ASSET_STATE_EMPTY;
    return;
  }
  auto mem = mem_alloc(f->GetSize());
  f->ReadBytes(mem->data, mem->size);
  FileSystem::Instance()->Close(f);

  SpinLockGuard lock_guard(&asset->_lock);
  asset->_header = (AssetMemoryHeader*)C3_ALLOC(g_allocator, ASSET_MEMORY_SIZE(0, sizeof(Texture)));
  asset->_header->_size = ASSET_MEMORY_SIZE(0, sizeof(Texture));
  asset->_header->_num_depends = 0;
  auto texture = (Texture*)asset->_header->GetData();
  texture->_handle = GraphicsRenderer::Instance()->CreateTexture(mem, C3_TEXTURE_NONE, 0, &texture->_info);
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
