#pragma once
#include "Platform/PlatformConfig.h"
#include "Data/DataType.h"

#define SORT_KEY_RENDER_DRAW (UINT64_C(1) << 0x36)
#define SORT_KEY_VIEW_SHIFT  UINT8_C(0x37)
#define SORT_KEY_VIEW_MASK   (u64(C3_MAX_VIEWS) << SORT_KEY_VIEW_SHIFT)

struct SortKey {
  u32 depth;
  u16 program;
  u16 seq;
  u8 view;
  u8 trans;

  u64 EncodeDraw() {
    // 1) non-transparent
    // |               3               2               1               0|
    // |fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210|
    // | vvvvvvvvdsssssssssssttpppppppppdddddddddddddddddddddddddddddddd|
    // |        ^^          ^ ^        ^                               ^|
    // |        ||          | |        |                               ||
    // |   view-+|      seq-+ +-trans  +-program                 depth-+|
    // |         +-draw                                                 |

    // 2) transparent(blend enabled)
    // |               3               2               1               0|
    // |fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210|
    // | vvvvvvvvdsssssssssssttddddddddddddddddddddddddddddddddppppppppp|
    // |        ^^          ^ ^        ^                               ^|
    // |        ||          | |        |                               ||
    // |   view-+|      seq-+ +-trans  +-depth                 program-+|
    // |         +-draw                                                 |

    const u64 trans_k = u64(trans) << 0x29;
    const u64 seq_k = u64(seq) << 0x2b;
    const u64 view_k = u64(view) << SORT_KEY_VIEW_SHIFT;
    const u64 depth_k = trans ? (u64(depth) << 9) : depth;
    const u64 program_k = trans ? (program  & (C3_MAX_PROGRAMS - 1)) : u64(program & (C3_MAX_PROGRAMS - 1)) << 0x20;
    const u64 key = depth_k | program_k | trans_k | SORT_KEY_RENDER_DRAW | seq_k | view_k;
    return key;
  }

  u64 EncodeCompute() {
    // |               3               2               1               0|
    // |fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210|
    // | vvvvvvvvdsssssssssssppppppppp                                  |
    // |        ^^          ^        ^                                  |
    // |        ||          |        |                                  |
    // |   view-+|      seq-+        +-program                          |
    // |         +-draw                                                 |

    const u64 program_k = u64(program  & (C3_MAX_PROGRAMS - 1)) << 0x22;
    const u64 seq_k = u64(seq) << 0x2b;
    const u64 view_k = u64(view) << SORT_KEY_VIEW_SHIFT;
    const u64 key = program_k | seq_k | view_k;
    return key;
  }

  /// Returns true if item is command.
  bool Decode(u64 key, u8 view_remap[C3_MAX_VIEWS]) {
    bool compute = _Decode(key);
    for (u8 i = 0; i < C3_MAX_VIEWS; ++i) {
      if (view_remap[i] == view) {
        view = i;
        break;
      }
    }
    //view = view_remap[view];
    return compute;
  }

  static u64 RemapView(u64 key, u8 view_remap[C3_MAX_VIEWS]) {
    const u8 old_view = u8((key & SORT_KEY_VIEW_MASK) >> SORT_KEY_VIEW_SHIFT);
    const u64 view = u64(view_remap[old_view]) << SORT_KEY_VIEW_SHIFT;
    const u64 new_key = (key & ~SORT_KEY_VIEW_MASK) | view;
    return new_key;
  }

  void Reset() {
    depth = 0;
    program = 0;
    seq = 0;
    view = 0;
    trans = 0;
  }

private:
  bool _Decode(u64 key) {
    seq = (key >> 0x2b) & 0x7ff;
    view = u8((key & SORT_KEY_VIEW_MASK) >> SORT_KEY_VIEW_SHIFT);
    if (key & SORT_KEY_RENDER_DRAW) {
      trans = (key >> 0x29) & 0x3;
      depth = trans ? ((key >> 9) & 0xffffffff) : (key & 0xffffffff);
      program = (trans ? key : (key >> 0x20)) & (C3_MAX_PROGRAMS - 1);
      if (program == C3_MAX_PROGRAMS - 1) program = UINT16_MAX;
      return false; // draw
    }
    program = (key >> 0x22) & (C3_MAX_PROGRAMS - 1);
    if (program == C3_MAX_PROGRAMS - 1) program = UINT16_MAX;
    return true; // compute
  }
};
#undef SORT_KEY_RENDER_DRAW
