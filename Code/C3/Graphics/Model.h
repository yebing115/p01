#pragma once
#include <MathGeoLib.h>
#include "Data/DataType.h"
#include "Data/String.h"
#include "Pattern/Handle.h"

class Material;
class ModelPart {
public:
  u32 _start_index;
  u32 _num_indices;
  int _material;
};

class Model {
public:
  Model();
  String _filename;
  Handle _vb;
  Handle _ib;
  vector<ModelPart> _parts;
  vector<Material*> _materials;
};
