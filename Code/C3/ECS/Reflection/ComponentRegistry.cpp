#include "ComponentRegistry.h"

unordered_map<u16, ComponentInfo> s_info_map;
unordered_map<stringid, u16> s_name_map;

void ComponentRegistry::Add(u16 type, IComponentProperty* prop) {
  auto& info = s_info_map[type];
  info._properties.push_back(prop);
}

void ComponentRegistry::SetName(u16 type, const char* name) {
  auto& info = s_info_map[type];
  info._name = name;
  s_name_map[String::GetID(name)] = type;
}

ComponentInfo* ComponentRegistry::GetByName(stringid name) {
  auto it = s_name_map.find(name);
  if (it == s_name_map.end()) return nullptr;
  return &s_info_map[it->second];
}

ComponentInfo* ComponentRegistry::GetByType(u16 type) {
  auto it = s_info_map.find(type);
  if (it == s_info_map.end()) return nullptr;
  return &it->second;
}

void ComponentRegistry::Add(u16 type, IComponentMethod* method) {
  auto& info = s_info_map[type];
  info._methods.push_back(method);
}