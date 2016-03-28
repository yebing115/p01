#pragma once
#include "Data/DataType.h"
#include "Data/String.h"

enum ResourceState {
  RS_EMPTY,
  RS_LOADING,
  RS_UNLOADING,
  RS_READY,
};

class Resource {
public:
  Resource() : _ref_count(0), _state(RS_EMPTY) {}
  virtual ~Resource();
  virtual void Load() = 0;
  virtual void Unload() = 0;
private:
  u32 _ref_count;
  ResourceState _state;
  String _filename;
};
