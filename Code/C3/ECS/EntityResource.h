#pragma once
#include "Data/DataType.h"

#define C3_CHUNK_MAGIC_ENT MAKE_FOURCC('E', 'N', 'T', ' ')

/************************************************************************/
/* ComponentTypeResourceHeader                                          */
/* -----------------------                                              */
/*   u32 _entity_owners[_num_entities]                                  */
/*   u8 _data[_num_entities]                                            */
/************************************************************************/
struct ComponentTypeResourceHeader {
  u32 _type;
  u32 _size;
  u32 _num_entities;
  u32 _data_offset;
};

/************************************************************************/
/* Entity Resource layout:                                              */
/* -----------------------                                              */
/*   EntityResourceHeader                                               */
/*   AssetDesc _asset_refs[_num_asset_refs]                             */
/*   u32 _entity_parents[_num_entities]                                 */
/*   ComponentTypeResource _ct_res[_num_component_types]                */
/************************************************************************/
struct EntityResourceHeader {
  u32 _magic;                         // "ENT "
  u32 _num_asset_refs;
  u32 _num_entites;
  u32 _num_component_types;
  u32 _asset_refs_data_offset;
  u32 _entity_parents_data_offset;
  u32 _component_types_data_offset;
  void InitDataOffsets() {
    _asset_refs_data_offset = ALIGN_16(sizeof(EntityResourceHeader));
    _entity_parents_data_offset = ALIGN_16(_asset_refs_data_offset + _num_asset_refs * sizeof(AssetDesc));
    _component_types_data_offset = ALIGN_16(_entity_parents_data_offset + _num_entites * sizeof(u32));
  }
};

struct EntityResourceDeserializeContext {
  Asset** _assets;
  EntityHandle* _entities;
  EntityResourceHeader _header;
};
