#include "C3PCH.h"
#include "World.h"
#include "System.h"

DEFINE_SINGLETON_INSTANCE(World);
IMPLEMENT_REFLECT(World);

World::World() {}
World::~World() {}

COMPONENT_HANDLE World::CreateComponent(u16 type) {
  COMPONENT_HANDLE h;
  for (auto sys : _systems) {
    if (sys->CreateComponent(type, h)) break;
  }
  return h;
}

void World::DestroyComponent(COMPONENT_HANDLE handle) {
  for (auto sys : _systems) {
    if (sys->DestroyComponent(handle)) return;
  }
}

ISystem* World::GetSystem(u16 type) const {
  for (auto sys : _systems) {
    if (sys->OwnComponentType(type)) return sys;
  }
  return nullptr;
}
