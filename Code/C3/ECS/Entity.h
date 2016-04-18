#pragma once

#include "Memory/HandleAlloc.h"
#include "Data/list.h"

enum EntityFlag {
  ENTITY_FLAG_NONE = 0,
  ENTITY_FLAG_TRANSIENT = (1 << 0),
};

struct Entity {
  EntityHandle _parent;
  u32 _flags;
  list_head _sibling_link;
  list_head _child_list;

  void Init() {
    _parent = EntityHandle();
    _flags = ENTITY_FLAG_NONE;
    INIT_LIST_HEAD(&_sibling_link);
    INIT_LIST_HEAD(&_child_list);
  }
};
