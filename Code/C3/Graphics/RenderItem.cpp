#include "RenderItem.h"
#include "GraphicsTypes.h"

void RenderItem::Clear() {
  start_vertex = 0;
  num_vertices = 0;
  start_index = 0;
  num_indices = 0;
  flags = C3_STATE_NONE;
  program = 0;
  constant_begin = 0;
  constant_end = 0;
  scissor = UINT16_MAX;
  matrix = 0;
  rgba = 0;
  instance_data_offset = 0;
  instance_data_stride = 0;
  num_instances = 1;
  memset(bind, 0xff, sizeof(bind));
}
