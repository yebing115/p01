#pragma once

#include "SLParser.h"
#include <stdio.h>

void write_binary(ShaderNode* shader, const void* payload, size_t playload_size, FILE* f);
