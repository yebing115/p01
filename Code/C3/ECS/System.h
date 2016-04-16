#pragma once

#include "Reflection/Object.h"
#include "Memory/HandleAlloc.h"
#include "ComponentTypes.h"

class ISystem : public Object {
public:
  virtual bool OwnComponentType(HandleType type) const = 0;
  virtual void CreateComponent(EntityHandle entity, HandleType type) = 0;
  virtual void Update(float dt, bool paused) {}
  virtual void Render(float dt, bool paused) {}
private:
  SUPPORT_REFLECT(ISystem)
};

template<HandleType TYPE, u32 COUNT>
Handle<TYPE> cast_to(GenericHandle gh,
                     const HandleAlloc<TYPE, COUNT>& handle_alloc) {
  auto h = Handle<TYPE>(gh.ToRaw());
  if (handle_alloc.IsValid(h)) return h;
  return Handle<TYPE>();
}

template<HandleType TYPE, u32 COUNT>
Handle<TYPE> cast_to(GenericHandle gh,
                     const HandleAlloc<TYPE, COUNT>& handle_alloc,
                     const unordered_map<EntityHandle, Handle<TYPE>>& handle_map) {
  if (gh.type == TYPE) return cast_to<TYPE, COUNT>(gh, handle_alloc);
  else if (gh.IsEntity()) {
    auto it = handle_map.find(EntityHandle(gh.ToRaw()));
    if (it != handle_map.end()) return it->second;
  }
  return Handle<TYPE>();
}
