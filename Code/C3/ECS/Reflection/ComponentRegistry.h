#pragma once

#include "ComponentInfo.h"
#include "Data/DataType.h"
#include "Data/String.h"

namespace ComponentRegistry {

void Add(u16 type, IComponentProperty* prop);
void Add(u16 type, IComponentMethod* method);
void SetName(u16 type, const char* name);
ComponentInfo* GetByName(stringid name);
ComponentInfo* GetByName(const char* name) { return GetByName(String::GetID(name)); }
ComponentInfo* GetByType(u16 type);

}
