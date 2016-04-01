#pragma once
#include "Data/DataType.h"
#include "Data/String.h"

struct PlatformData {
  void* ndt;            //!< Native display type
  void* nwh;            //!< Native window handle
  void* context;        //!< GL context, or D3D device
  void* back_buffer;    //!< GL backbuffer, or D3D render target view
  void* back_buffer_ds; //!< Backbuffer depth/stencil.
  vector<String> arguments;
  bool use_archive;
};
extern PlatformData g_platform_data;
