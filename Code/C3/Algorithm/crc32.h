//
//  crc32.h
//  C2Engine
//
//  Created by mike luo on 2014-2-11.
//
//
#ifndef CRC_32_H
#define CRC_32_H

#include "Data/DataType.h"

extern u32 compute_crc32_length(const void* buffer, u32 length, u32 partial_crc = 0);
extern u32 compute_crc32_null(const char* buffer, u32 partial_crc = 0);

#endif // CRC_32_H