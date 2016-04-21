#include "EngineAPI.h"
#include "C3PCH.h"

DEFINE_SINGLETON_INSTANCE(EngineAPI);

EngineAPI::EngineAPI(HWND hwnd): debug_output(hwnd), _hwnd(hwnd) {
  setup_callback(hwnd);
  attach_dom_event_handler(hwnd, this);
  SciterSetOption(NULL, SCITER_SET_UX_THEMING, TRUE);
  SciterSetOption(hwnd, SCITER_SET_DEBUG_MODE, TRUE);
}

EngineAPI::~EngineAPI() {

}

sciter::value EngineAPI::GetEntityList() {
  sciter::value value_list = sciter::value::from_string(L"[]", 2, CVT_JSON_LITERAL);
  value_list.set_item(0, sciter::value());
  auto GW = GameWorld::Instance();
  int num_entities = GW->GetNumEntities();
  auto entities = (Entity*)C3_ALLOC(g_allocator, sizeof(Entity) * num_entities);
  GW->GetSortedEntities(entities, num_entities);
  for (int i = 0; i < num_entities; ++i) {
    auto e = entities + i;
    sciter::value entity_value = GetEntity(e->_handle);
    value_list.set_item(i, entity_value);
  }
  C3_FREE(g_allocator, entities);
  return value_list;
}

sciter::value EngineAPI::GetEntity(sciter::value eh_value) {
  if (!eh_value.is_int()) return sciter::value();
  EntityHandle e(eh_value.get<int>());
  return GetEntity(e);
}

sciter::value EngineAPI::GetEntity(EntityHandle e) {
  auto GW = GameWorld::Instance();
  auto mem = mem_alloc(256 * 1024);
  JsonWriter writer((char*)mem->data, mem->size);
  SerializeEntity(GW->FindEntity(e), writer);
  aux::utf2w chars((const char*)mem->data);
  sciter::value data_value = sciter::value::from_string(chars.c_str(), chars.length(), CVT_JSON_LITERAL);
  mem_free(mem);
  return data_value;
}

void EngineAPI::SerializeTransform(Transform* transform, JsonWriter& writer) {
  if (!transform) return;
  writer.BeginWriteObject("transform");
  writer.WriteInt("entity", (int)transform->_entity.ToRaw());
  writer.WriteFloat("x", transform->_position.x);
  writer.WriteFloat("y", transform->_position.y);
  writer.WriteFloat("z", transform->_position.z);
  auto r = transform->_rotation.ToEulerZXY();
  writer.WriteFloat("rx", r[1]);
  writer.WriteFloat("ry", r[2]);
  writer.WriteFloat("rz", r[0]);
  writer.WriteFloat("sx", transform->_scale.x);
  writer.WriteFloat("sy", transform->_scale.y);
  writer.WriteFloat("sz", transform->_scale.z);
  writer.EndWriteObject();
}

void EngineAPI::SerializeCamera(Camera* camera, JsonWriter& writer) {
  if (!camera) return;
  writer.BeginWriteObject("camera");
  writer.WriteInt("entity", (int)camera->_entity.ToRaw());
  auto pos = camera->GetPos();
  writer.WriteFloat("x", pos.x);
  writer.WriteFloat("y", pos.y);
  writer.WriteFloat("z", pos.z);
  auto front = camera->GetFront();
  auto up = camera->GetUp();
  auto right = camera->GetRight();
  float3 rotation = float3x3(right, up, front).ToEulerZXY();
  writer.WriteFloat("rx", RadToDeg(rotation[1]));
  writer.WriteFloat("ry", RadToDeg(rotation[2]));
  writer.WriteFloat("rz", RadToDeg(rotation[0]));
  writer.WriteFloat("near", camera->GetNear());
  writer.WriteFloat("far", camera->GetFar());
  writer.WriteFloat("v_fov", RadToDeg(camera->GetVerticalFov()));
  writer.WriteFloat("aspect", RadToDeg(camera->GetAspectRatio()));
  writer.EndWriteObject();
}

void EngineAPI::SerializeModel(ModelRenderer* mr, JsonWriter& writer) {
  if (!mr) return;
  writer.BeginWriteObject("model_renderer");
  writer.WriteInt("entity", (int)mr->_entity.ToRaw());
  if (!mr->_asset) writer.WriteString("asset", "");
  else writer.WriteString("asset", mr->_asset->_desc._filename);
  writer.EndWriteObject();
}

void EngineAPI::SerializeLight(Light* light, JsonWriter& writer) {
  if (!light) return;
  writer.BeginWriteObject("light");
  writer.WriteInt("entity", (int)light->_entity.ToRaw());
  writer.WriteFloat("x", light->_pos.x);
  writer.WriteFloat("y", light->_pos.y);
  writer.WriteFloat("z", light->_pos.z);
  writer.WriteFloat("dx", light->_dir.x);
  writer.WriteFloat("dy", light->_dir.y);
  writer.WriteFloat("dz", light->_dir.z);
  writer.WriteFloat("r", light->_color.GetRed());
  writer.WriteFloat("g", light->_color.GetGreen());
  writer.WriteFloat("b", light->_color.GetBlue());
  writer.WriteFloat("a", light->_color.GetAlpha());
  writer.EndWriteObject();
}

void EngineAPI::OnEntityCreate(EntityHandle e) {
  auto data = GetEntity(e);
  call_function("EngineCallback.OnEntityCreate", data);
}

void EngineAPI::SerializeEntity(Entity* e, JsonWriter& writer) {
  if (!e) return;
  writer.BeginWriteObject();

  writer.WriteInt("id", (int)e->_handle.ToRaw());
  writer.WriteInt("parent", (int)e->_parent.ToRaw());
  writer.WriteString("name", GameWorld::Instance()->GetEntityName(e->_handle));

  auto GW = GameWorld::Instance();
  SerializeTransform(GW->FindTransform(e->_handle), writer);
  SerializeCamera(GW->FindCamera(e->_handle), writer);
  auto renderer = (RenderSystem*)GW->GetSystem(MODEL_RENDERER_COMPONENT);
  if (renderer) {
    SerializeModel(renderer->FindModel(e->_handle), writer);
    SerializeLight(renderer->FindLight(e->_handle), writer);
  }

  writer.EndWriteObject();
}

sciter::value EngineAPI::GetComponent(sciter::value eh_value, sciter::value comp_type_name_value) {
  sciter::value comp_data;
  if (!eh_value.is_int() || !comp_type_name_value.is_string()) return comp_data;
  EntityHandle eh((u32)eh_value.get<int>());
  ComponentType type;
  String comp_name = aux::w2utf(comp_type_name_value.get_chars()).c_str();
  if (comp_name.Equal("transform")) type = TRANSFORM_COMPONENT;
  else if (comp_name.Equal("camera")) type = CAMERA_COMPONENT;
  else if (comp_name.Equal("model_renderer")) type = MODEL_RENDERER_COMPONENT;
  else if (comp_name.Equal("light")) type = LIGHT_COMPONENT;
  else return comp_data;
  comp_data = GetComponent(eh, type);
  return comp_data;
}

sciter::value EngineAPI::GetComponent(EntityHandle eh, ComponentType type) {
  auto mem = mem_alloc(1024);
  JsonWriter writer((char*)mem->data, mem->size);
  auto GW = GameWorld::Instance();
  writer.BeginWriteObject();
  if (type == TRANSFORM_COMPONENT) SerializeTransform(GW->FindTransform(eh), writer);
  else if (type == CAMERA_COMPONENT) SerializeCamera(GW->FindCamera(eh), writer);
  else if (type == MODEL_RENDERER_COMPONENT) {
    auto renderer = (RenderSystem*)GW->GetSystem(type);
    SerializeModel(renderer->FindModel(eh), writer);
  } else if (type == LIGHT_COMPONENT) {
    auto renderer = (RenderSystem*)GW->GetSystem(type);
    SerializeLight(renderer->FindLight(eh), writer);
  }
  writer.EndWriteObject();
  aux::utf2w cvt((const char*)mem->data);
  return sciter::value::from_string(cvt.c_str(), cvt.length(), CVT_JSON_LITERAL);
}

bool EngineAPI::LoadWorld(sciter::value filename_value) {
  if (!filename_value.is_string()) return false;
  String filename = aux::w2utf(filename_value.get_chars()).c_str();
  if (filename.StartsWith("file://")) filename = filename.Substr(7);
  auto file = new CrtFile(filename);
  if (!file || !file->IsValid()) {
    delete file;
    return false;
  }
  auto n = file->GetSize();
  auto mem = mem_alloc(n + 1);
  file->ReadBytes(mem->data, n);
  delete file;
  *((u8*)mem->data + n) = 0;
  JsonReader reader((const char*)mem->data);
  _deserialize_entity_map.clear();
  if (reader.BeginReadArray("entities")) {
    while (reader.BeginReadObject()) {
      EntityHandle e = DeserializeEntity(reader);
      reader.EndReadObject();
    }
    reader.EndReadArray();
  }
  mem_free(mem);
  return true;
}

bool EngineAPI::SaveWorld(sciter::value filename_value) {
  if (!filename_value.is_string()) return false;
  aux::w2utf cvt(filename_value.get_chars());
  auto file = FileSystem::Instance()->OpenWrite(cvt.c_str());
  if (!file) return false;
  auto GW = GameWorld::Instance();
  int num_entities = GW->GetNumEntities();
  auto entities = (Entity*)C3_ALLOC(g_allocator, sizeof(Entity) * num_entities);
  GW->GetSortedEntities(entities, num_entities);
  auto mem = mem_alloc((num_entities + 1) * 1024);
  JsonWriter writer((char*)mem->data, mem->size);
  for (int i = 0; i < num_entities; ++i) {
    SerializeEntity(entities + i, writer);
  }
  C3_FREE(g_allocator, entities);
  int n = strlen((char*)mem->data);
  file->WriteBytes(mem->data, n);
  FileSystem::Instance()->Close(file);
  mem_free(mem);
  return true;
}

void EngineAPI::OnEntityDestroy(EntityHandle eh) {
  call_function("EngineCallback.OnEntityDestroy", (int)eh.ToRaw());
}

void EngineAPI::OnEntityReparent(EntityHandle eh, EntityHandle new_parent) {
  call_function("EngineCallback.OnEntityReparent", (int)eh.ToRaw(), (int)new_parent.ToRaw());
}

void EngineAPI::OnComponentCreate(EntityHandle eh, ComponentType type) {
  call_function("EngineCallback.OnComponentCreate", (int)eh.ToRaw(), GetComponent(eh, type));
}

void EngineAPI::OnComponentDestroy(EntityHandle eh, ComponentType type) {
  call_function("EngineCallback.OnComponentDestroy", (int)eh.ToRaw(), GetComponentTypeName(type));
}

void EngineAPI::OnComponentChange(EntityHandle eh, ComponentType type) {
  call_function("EngineCallback.OnComponentChange", (int)eh.ToRaw(), GetComponent(eh, type));
}

EntityHandle EngineAPI::DeserializeEntity(JsonReader& reader) {
  auto GW = GameWorld::Instance();
  EntityHandle e;
  int e_id;
  int p_id;
  reader.ReadInt("id", e_id);
  reader.ReadInt("parent", p_id, -1);
  if (p_id == -1) e = GW->CreateEntity();
  else {
    auto it = _deserialize_entity_map.find(p_id);
    if (it == _deserialize_entity_map.end()) e = GW->CreateEntity();
    else e = GW->CreateEntity(it->second);
  }
  if (e) {
    _deserialize_entity_map[e_id] = e;
    char name[1024];
    reader.ReadString("name", name, sizeof(name));
    if (strlen(name) > 0) GW->SetEntityName(e, name);
    OnEntityCreate(e);
    DeserializeTransform(e, reader);
    DeserializeCamera(e, reader);
    DeserializeModel(e, reader);
    DeserializeLight(e, reader);
  }
  return e;
}

void EngineAPI::DeserializeTransform(EntityHandle e, JsonReader& reader) {
  if (reader.BeginReadObject("transform")) {
    auto GW = GameWorld::Instance();
    auto transform = GW->CreateTransform(e);
    reader.ReadFloat("x", transform->_position.x);
    reader.ReadFloat("y", transform->_position.y);
    reader.ReadFloat("z", transform->_position.z);
    float rx, ry, rz;
    reader.ReadFloat("rx", rx);
    reader.ReadFloat("ry", ry);
    reader.ReadFloat("rz", rz);
    transform->_rotation = Quat::FromEulerZXY(rz, rx, ry);
    reader.ReadFloat("sx", transform->_scale.x);
    reader.ReadFloat("sy", transform->_scale.y);
    reader.ReadFloat("sz", transform->_scale.z);
    OnComponentCreate(e, TRANSFORM_COMPONENT);
    reader.EndReadObject();
  }
}

void EngineAPI::DeserializeCamera(EntityHandle e, JsonReader& reader) {
  if (reader.BeginReadObject("camera")) {
    auto GW = GameWorld::Instance();
    auto camera = GW->CreateCamera(e);
    float3 pos;
    reader.ReadFloat("x", pos.x);
    reader.ReadFloat("y", pos.y);
    reader.ReadFloat("z", pos.z);
    camera->SetPos(pos);
    float rx, ry, rz;
    reader.ReadFloat("rx", rx);
    reader.ReadFloat("ry", ry);
    reader.ReadFloat("rz", rz);
    float3x3 rotation_matrix = float3x3::FromEulerZXY(DegToRad(rz), DegToRad(rx), DegToRad(ry));
    camera->SetFront(rotation_matrix.Col(2));
    camera->SetUp(rotation_matrix.Col(1));
    float near, far;
    reader.ReadFloat("near", near);
    reader.ReadFloat("far", far);
    camera->SetClipPlane(near, far);
    float v_fov, aspect;
    reader.ReadFloat("v_fov", v_fov);
    reader.ReadFloat("aspect", aspect);
    camera->SetVerticalFovAndAspectRatio(DegToRad(v_fov), aspect);
    OnComponentCreate(e, CAMERA_COMPONENT);
    reader.EndReadObject();
  }
}

void EngineAPI::DeserializeModel(EntityHandle e, JsonReader& reader) {
  if (reader.BeginReadObject("model_renderer")) {
    auto GW = GameWorld::Instance();
    auto renderer = (RenderSystem*)GameWorld::Instance()->GetSystem(MODEL_RENDERER_COMPONENT);
    if (renderer) {
      auto mr = renderer->CreateModelRenderer(e);
      if (mr->_asset) AssetManager::Instance()->Unload(mr->_asset);
      char filename[MAX_ASSET_NAME];
      reader.ReadString("asset", filename, sizeof(filename));
      mr->_asset = AssetManager::Instance()->Load(ASSET_TYPE_MODEL, filename);
      OnComponentCreate(e, MODEL_RENDERER_COMPONENT);
    }
    reader.EndReadObject();
  }
}

void EngineAPI::DeserializeLight(EntityHandle e, JsonReader& reader) {
  if (reader.BeginReadObject("light")) {
    auto GW = GameWorld::Instance();
    auto renderer = (RenderSystem*)GameWorld::Instance()->GetSystem(MODEL_RENDERER_COMPONENT);
    if (renderer) {
      auto light = renderer->CreateLight(e);
      reader.ReadFloat("x", light->_pos.x);
      reader.ReadFloat("y", light->_pos.y);
      reader.ReadFloat("z", light->_pos.z);
      float r, g, b, a;
      reader.ReadFloat("r", r);
      reader.ReadFloat("g", g);
      reader.ReadFloat("b", b);
      reader.ReadFloat("a", a);
      light->_color = Color(r, g, b, a);
      OnComponentCreate(e, LIGHT_COMPONENT);
    }
    reader.EndReadObject();
  }
}

const char* EngineAPI::GetComponentTypeName(ComponentType type) const {
  switch (type) {
  case TRANSFORM_COMPONENT:
    return "transform";
  case CAMERA_COMPONENT:
    return "camera";
  case MODEL_RENDERER_COMPONENT:
    return "model_renderer";
  case LIGHT_COMPONENT:
    return "light";
  default:
    return "";
  }
}

sciter::value EngineAPI::SetRootDir(sciter::value dir_value) {
  if (!dir_value.is_string()) return sciter::value(false);
  String dir = aux::w2utf(dir_value.get_chars()).c_str();
  if (dir.StartsWith("file://")) dir = dir.Substr(7);
  FileSystem::Instance()->SetRootDir(dir.GetCString());
}
