#pragma once

#include "ComponentInfo.h"
#include "Data/DataType.h"
#include "Data/String.h"

namespace ComponentRegistry {

extern void Add(u16 type, IComponentProperty* prop);
extern void Add(u16 type, IComponentMethod* method);
extern void SetName(u16 type, const char* name);
extern ComponentInfo* GetByName(stringid name);
inline ComponentInfo* GetByName(const char* name) { return GetByName(String::GetID(name)); }
extern ComponentInfo* GetByType(u16 type);

}
