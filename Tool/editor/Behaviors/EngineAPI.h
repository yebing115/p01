#pragma once
#include "Pattern/Singleton.h"
#include "Platform/Windows/WindowsHeader.h"
#include "ECS/Entity.h"
#include "ECS/ComponentTypes.h"
#include "Data/Json.h"
#include <sciter-x.h>

struct Transform;
struct Camera;
struct ModelRenderer;
struct Light;
class EngineAPI : public sciter::host<EngineAPI>
                , public sciter::event_handler
                , public sciter::debug_output {
public:
  EngineAPI(HWND hwnd);
  ~EngineAPI();

  // sciter::host traits
  HWINDOW get_hwnd() const { return _hwnd; }
  HINSTANCE get_resource_instance() const { return GetModuleHandle(NULL); }

  // Script interfaces
  sciter::value SetRootDir(sciter::value dir_value);
  bool LoadWorld(sciter::value filename);
  bool SaveWorld(sciter::value filename);
  sciter::value GetEntityList();
  sciter::value GetEntity(sciter::value eh_value);
  sciter::value GetComponent(sciter::value eh_value, sciter::value comp_value);

  // Callbacks
  void OnEntityCreate(EntityHandle eh);
  void OnEntityDestroy(EntityHandle eh);
  void OnEntityReparent(EntityHandle eh, EntityHandle new_parent);
  void OnComponentCreate(EntityHandle eh, ComponentType type);
  void OnComponentDestroy(EntityHandle eh, ComponentType type);
  void OnComponentChange(EntityHandle eh, ComponentType type);

  BEGIN_FUNCTION_MAP
    FUNCTION_1("c3_set_root_dir", SetRootDir)
    FUNCTION_1("c3_load_world", LoadWorld)
    FUNCTION_1("c3_save_world", SaveWorld)
    FUNCTION_0("c3_get_entity_list", GetEntityList)
    FUNCTION_1("c3_get_entity", GetEntity)
    FUNCTION_2("c3_get_component", GetComponent)
  END_FUNCTION_MAP

private:
  const char* GetComponentTypeName(ComponentType type) const;
  void SerializeEntity(Entity* entity, JsonWriter& writer);
  void SerializeTransform(Transform* transform, JsonWriter& writer);
  void SerializeCamera(Camera* camera, JsonWriter& writer);
  void SerializeModel(ModelRenderer* mr, JsonWriter& writer);
  void SerializeLight(Light* light, JsonWriter& writer);
  EntityHandle DeserializeEntity(JsonReader& reader);
  void DeserializeTransform(EntityHandle e, JsonReader& reader);
  void DeserializeCamera(EntityHandle e, JsonReader& reader);
  void DeserializeModel(EntityHandle e, JsonReader& reader);
  void DeserializeLight(EntityHandle e, JsonReader& reader);
  sciter::value GetEntity(EntityHandle eh);
  sciter::value GetComponent(EntityHandle eh, ComponentType type);

  HWND _hwnd;
  unordered_map<int, EntityHandle> _deserialize_entity_map;

  SUPPORT_SINGLETON_WITH_ONE_ARG_CREATOR(EngineAPI, HWND);
};