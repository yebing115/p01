#pragma once

#include "Reflection/Object.h"
#include "ComponentTypes.h"

class ISystem : public Object {
public:
  virtual bool OwnHandleType(HandleType type) = 0;

private:
  SUPPORT_REFLECT(ISystem)
};
