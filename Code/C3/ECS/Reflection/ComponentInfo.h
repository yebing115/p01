#pragma once
#include "ComponentProperty.h"
#include "ComponentMethod.h"

class ComponentInfo {
public:
  u16 _type;
  const char* _name;
  vector<IComponentProperty*> _properties;
  vector<IComponentMethod*> _methods;
};

