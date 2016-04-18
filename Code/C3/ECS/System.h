#pragma once

#include "Reflection/Object.h"
#include "Memory/HandleAlloc.h"
#include "ComponentTypes.h"

class BlobWriter;
class ISystem : public Object {
public:
  virtual bool OwnComponentType(ComponentType type) const = 0;
  virtual void CreateComponent(EntityHandle entity, ComponentType type) = 0;
  virtual void SerializeComponents(BlobWriter& writer) {}
  virtual void Update(float dt, bool paused) {}
  virtual void Render(float dt, bool paused) {}
private:
  SUPPORT_REFLECT(ISystem)
};
