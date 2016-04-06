#pragma once

#include "Memory/HandleAlloc.h"
#include "Data/list.h"

struct Entity {
  struct Entity* _parent;
  list_head _sibling_link;
  list_head _child_list;

  void Init() {
    _parent = nullptr;
    INIT_LIST_HEAD(&_sibling_link);
    INIT_LIST_HEAD(&_child_list);
  }
};
