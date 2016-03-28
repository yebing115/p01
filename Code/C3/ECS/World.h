#pragma once

#include "Data/DataType.h"
#include "Reflection/Object.h"
#include "Component.h"
#include "Pattern/Singleton.h"

class ISystem;
class World : public Object {
public:
  World();
  ~World();
  COMPONENT_HANDLE CreateComponent(u16 type);
  void DestroyComponent(COMPONENT_HANDLE handle);

  void AddSystem(ISystem* system) { _systems.push_back(system); }
  ISystem* GetSystem(u16 type) const;

  static World* GetCurrent();

private:
  vector<ISystem*> _systems;

  SUPPORT_REFLECT(World)
  SUPPORT_SINGLETON(World)
};
