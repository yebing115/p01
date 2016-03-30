#include "VertexFormat.h"

const char* VERTEX_ATTR_NAMES[VERTEX_ATTR_COUNT] = {
  "POSITION",
  "NORMAL",
  "TANGENT",
  "BITANGENT",
  "COLOR0",
  "COLOR1",
  "TEXCOORD0",
  "TEXCOORD1",
  "INDEX",
  "WEIGHT",
};

VertexDecl VERTEX_DECLS[VERTEX_FORMAT_COUNT];

void init_builtin_vertex_decls(GraphicsAPI api) {
  {
    auto& vd = VERTEX_DECLS[VERTEX_P2];
    vd.Begin(api);
    vd.Add(VERTEX_ATTR_POSITION, 2, FLOAT_TYPE, false, false);
    vd.End();
  }
  {
    auto& vd = VERTEX_DECLS[VERTEX_P2_T2];
    vd.Begin(api);
    vd.Add(VERTEX_ATTR_POSITION, 2, FLOAT_TYPE, false, false);
    vd.Add(VERTEX_ATTR_TEXCOORD0, 2, FLOAT_TYPE, false, false);
    vd.End();
  }
  {
    auto& vd = VERTEX_DECLS[VERTEX_P3];
    vd.Begin(api);
    vd.Add(VERTEX_ATTR_POSITION, 3, FLOAT_TYPE, false, false);
    vd.End();
  }
  {
    auto& vd = VERTEX_DECLS[VERTEX_P3_T2];
    vd.Begin(api);
    vd.Add(VERTEX_ATTR_POSITION, 3, FLOAT_TYPE, false, false);
    vd.Add(VERTEX_ATTR_TEXCOORD0, 2, FLOAT_TYPE, false, false);
    vd.End();
  }
  {
    auto& vd = VERTEX_DECLS[VERTEX_P3_T2_C4];
    vd.Begin(api);
    vd.Add(VERTEX_ATTR_POSITION, 3, FLOAT_TYPE, false, false);
    vd.Add(VERTEX_ATTR_TEXCOORD0, 2, FLOAT_TYPE, false, false);
    vd.Add(VERTEX_ATTR_COLOR0, 4, FLOAT_TYPE, false, false);
    vd.End();
  }
  {
    auto& vd = VERTEX_DECLS[VERTEX_P3_N3];
    vd.Begin(api);
    vd.Add(VERTEX_ATTR_POSITION, 3, FLOAT_TYPE, false, false);
    vd.Add(VERTEX_ATTR_NORMAL, 3, FLOAT_TYPE, false, false);
    vd.End();
  }
  {
    auto& vd = VERTEX_DECLS[VERTEX_P3_N3_T2];
    vd.Begin(api);
    vd.Add(VERTEX_ATTR_POSITION, 3, FLOAT_TYPE, false, false);
    vd.Add(VERTEX_ATTR_NORMAL, 3, FLOAT_TYPE, false, false);
    vd.Add(VERTEX_ATTR_TEXCOORD0, 2, FLOAT_TYPE, false, false);
    vd.End();
  }
  {
    auto& vd = VERTEX_DECLS[VERTEX_P2_T2_I1];
    vd.Begin(api);
    vd.Add(VERTEX_ATTR_POSITION, 2, FLOAT_TYPE, false, false);
    vd.Add(VERTEX_ATTR_TEXCOORD0, 2, FLOAT_TYPE, false, false);
    vd.Add(VERTEX_ATTR_INDEX, 4, INT8_TYPE, false, true);
    vd.End();
  }
  {
    auto& vd = VERTEX_DECLS[VERTEX_P3_T2_J4];
    vd.Begin(api);
    vd.Add(VERTEX_ATTR_POSITION, 3, FLOAT_TYPE, false, false);
    vd.Add(VERTEX_ATTR_TEXCOORD0, 2, FLOAT_TYPE, false, false);
    vd.Add(VERTEX_ATTR_INDEX, 4, INT8_TYPE, false, true);
    vd.Add(VERTEX_ATTR_WEIGHT, 4, FLOAT_TYPE, false, false);
    vd.End();
  }
}

struct AttribToSemantic {
  int attr;
  stringid semantic;
};

static AttribToSemantic s_attrib_to_semantic[] = {
  // NOTICE:
  // Attrib must be in order how it appears in Attrib::Enum! id is
  // unique and should not be changed if new Attribs are added.
  {VERTEX_ATTR_POSITION, hash_string("POSITION")},
  {VERTEX_ATTR_NORMAL, hash_string("NORMAL")},
  {VERTEX_ATTR_TANGENT, hash_string("TANGENT")},
  {VERTEX_ATTR_BITANGENT, hash_string("BITANGENT")},
  {VERTEX_ATTR_COLOR0, hash_string("COLOR0")},
  {VERTEX_ATTR_COLOR1, hash_string("COLOR1")},
  {VERTEX_ATTR_TEXCOORD0, hash_string("TEXCOORD0")},
  {VERTEX_ATTR_TEXCOORD1, hash_string("TEXCOORD1")},
  {VERTEX_ATTR_INDEX, hash_string("INDEX")},
  {VERTEX_ATTR_WEIGHT, hash_string("WEIGHT")},
  {INSTANCE_ATTR_I_DATA0, hash_string("I_DATA0")},
  {INSTANCE_ATTR_I_DATA1, hash_string("I_DATA1")},
  {INSTANCE_ATTR_I_DATA2, hash_string("I_DATA2")},
  {INSTANCE_ATTR_I_DATA3, hash_string("I_DATA3")},
  {INSTANCE_ATTR_I_DATA4, hash_string("I_DATA4")},
};

int semantic_to_vertex_attr(stringid semantic) {
  for (u32 ii = 0; ii < ARRAY_SIZE(s_attrib_to_semantic); ++ii) {
    if (s_attrib_to_semantic[ii].semantic == semantic) {
      return s_attrib_to_semantic[ii].attr;
    }
  }

  return VERTEX_ATTR_COUNT + INSTANCE_ATTR_COUNT;
}

stringid vertex_attr_to_semantic(int attr) {
  return s_attrib_to_semantic[attr].semantic;
}
