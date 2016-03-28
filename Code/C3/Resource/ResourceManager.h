#pragma once
#include "Data/DataType.h"
#include "Pattern/Singleton.h"
#include "Resource.h"

enum ResourceType {
  RT_SHADER,
  RT_MATERIAL,
  RT_MODEL,
  RT_TEXTURE,
};

class ResourceLoader;
class ResourceManager final {
public:
  ResourceLoader* GetLoader(ResourceType type) const;
  void AddLoader(ResourceType type, ResourceLoader* loader);

private:
  unordered_map<ResourceType, ResourceLoader*> _loaders;
  SUPPORT_SINGLETON(ResourceManager)
};