#pragma once

#include "Reflection/Object.h"
#include "Component.h"

class ISystem : public Object {
public:
  virtual bool OwnComponentType(u16 type) = 0;
  virtual bool CreateComponent(u16 type, COMPONENT_HANDLE& out_handle) = 0;
  virtual bool DestroyComponent(COMPONENT_HANDLE handle) = 0;

private:
  SUPPORT_REFLECT(ISystem)
};
