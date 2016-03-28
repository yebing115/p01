#include "ResourceManager.h"

DEFINE_SINGLETON_INSTANCE(ResourceManager);

ResourceLoader * ResourceManager::GetLoader(ResourceType type) const {
  auto it = _loaders.find(type);
  if (it == _loaders.end()) return nullptr;
  else return it->second;
}

void ResourceManager::AddLoader(ResourceType type, ResourceLoader * loader) {
  _loaders[type] = loader;
}
