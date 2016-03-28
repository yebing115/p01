#pragma once
#include "Data/DataType.h"
#include "Data/String.h"

class Resource;
class ResourceLoader {
public:
  ResourceLoader();
  virtual ~ResourceLoader();

  Resource* Get(const String& filename);
  Resource* Load(const String& filename);
  void Add(Resource* resource);
  void Remove(Resource* resource);
  void Load(Resource& resource);
  void RemoveUnused();

  void Unload(const String& String);
  void Unload(Resource& resource);

  void ForceUnload(const String& String);
  void ForceUnload(Resource& resource);

  void Reload(const String& String);
  void Reload(Resource& resource);

protected:
  virtual Resource* CreateResource(const String& String) = 0;
  virtual void DestroyResource(Resource& resource) = 0;
};
