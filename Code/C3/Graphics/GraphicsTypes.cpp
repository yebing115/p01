#include "C3PCH.h"
#include "GraphicsTypes.h"
#include "Algorithm/Hasher.h"

const u32 PREDEFINED_CONSTANT_NAME[PREDEFINED_CONSTANT_COUNT] = {
  hash_string("u_view_rect"),
  hash_string("u_view_texel"),
  hash_string("u_view"),
  hash_string("u_inv_view"),
  hash_string("u_proj"),
  hash_string("u_inv_proj"),
  hash_string("u_view_proj"),
  hash_string("u_inv_view_proj"),
  hash_string("u_model"),
  hash_string("u_inv_model"),
  hash_string("u_model_view"),
  hash_string("u_model_view_proj"),
  hash_string("u_alpha_ref"),
  hash_string("u_time"),
};
