#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "Data/DataType.h"
#include "Data/String.h"
#include "Graphics/GraphicsTypes.h"
#include "Graphics/VertexFormat.h"
#include "Graphics/GraphicsRenderer.h"
#include "Graphics/Model/Mesh.h"

static const stringid INPUT_TYPE_OBJ = String::GetID("obj");
static const stringid INPUT_TYPE_FBX = String::GetID("fbx");

struct ProgramOption {
  stringid input_type;
  String input_filename;
  String output_filename;
  String model_name;
  u32 attr_mask;
  u32 verbose;
} g_options;


void error(const char* fmt, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  puts(buf);
  exit(-1);
}

void copy_vertex_attr(u8* buf, u32 num, u16 stride, const void* attr_data, size_t elem_size) {
  const u8* src = (const u8*)attr_data;
  for (u32 i = 0; i < num; ++i) {
    memcpy(buf, src, elem_size);
    buf += stride;
    src += elem_size;
  }
}

void process(const aiScene* scene) {
  MeshHeader header;
  memset(&header, 0, sizeof(header));
  header.magic = C3_CHUNK_MAGIC_MEX;
  header.aabb.SetNegativeInfinity();
  vector<MeshPart> parts;
  VertexDecl decl;
  unordered_set<String> part_names;
  unordered_set<String> material_names;
  unordered_map<String, int> dup_part_names;
  if (scene->mNumMeshes == 0) error("No mesh.\n");
  aiMesh* first_mesh = scene->mMeshes[0];
  auto attr = header.attrs;
  attr->attr = VERTEX_ATTR_POSITION;
  attr->num = 3;
  attr->data_type = FLOAT_TYPE;
  attr->flags = MESH_ATTR_DEFAULT;
  ++attr;
  decl.Begin().Add(VERTEX_ATTR_POSITION, 3, FLOAT_TYPE);
  if (first_mesh->HasNormals()) {
    decl.Add(VERTEX_ATTR_NORMAL, 3, FLOAT_TYPE);
    attr->attr = VERTEX_ATTR_NORMAL;
    attr->num = 3;
    attr->data_type = FLOAT_TYPE;
    attr->flags = MESH_ATTR_DEFAULT;
    ++attr;
}
#if 0
  if (first_mesh->HasTangentsAndBitangents()) {
    decl.Add(VERTEX_ATTR_TANGENT, 3, FLOAT_TYPE);
    decl.Add(VERTEX_ATTR_BITANGENT, 3, FLOAT_TYPE);
  }
#endif
  if (first_mesh->HasTextureCoords(0)) {
    decl.Add(VERTEX_ATTR_TEXCOORD0, 2, FLOAT_TYPE);
    attr->attr = VERTEX_ATTR_TEXCOORD0;
    attr->num = 2;
    attr->data_type = FLOAT_TYPE;
    attr->flags = MESH_ATTR_DEFAULT;
    ++attr;
  }
  if (first_mesh->HasTextureCoords(1)) {
    decl.Add(VERTEX_ATTR_TEXCOORD1, 2, FLOAT_TYPE);
    attr->attr = VERTEX_ATTR_TEXCOORD1;
    attr->num = 2;
    attr->data_type = FLOAT_TYPE;
    attr->flags = MESH_ATTR_DEFAULT;
    ++attr;
  }
  if (first_mesh->HasVertexColors(0)) {
    decl.Add(VERTEX_ATTR_COLOR0, 4, FLOAT_TYPE);
    attr->attr = VERTEX_ATTR_COLOR0;
    attr->num = 4;
    attr->data_type = FLOAT_TYPE;
    attr->flags = MESH_ATTR_DEFAULT;
    ++attr;
  }
  if (first_mesh->HasVertexColors(1)) {
    decl.Add(VERTEX_ATTR_COLOR1, 4, FLOAT_TYPE);
    attr->attr = VERTEX_ATTR_COLOR1;
    attr->num = 4;
    attr->data_type = FLOAT_TYPE;
    attr->flags = MESH_ATTR_DEFAULT;
    ++attr;
  }
  decl.End();
  header.vertex_stride = decl.stride;
  header.num_attrs = attr - header.attrs;

  u8* vertex_buf = nullptr;
  u32 num_vertices = 0;
  u8* index_buf = nullptr;
  u32 num_indices = 0;
  u32 index_size = 2;
  auto patch_part_name = [](const unordered_set<String>& names, unordered_map<String, int>& dups,
                            String& part_name, const String& last_part_name) {
    if (names.count(part_name) == 0 && part_name != "defaultobject") return;
    auto dup_it = dups.find(last_part_name);
    if (dup_it == dups.end()) {
      dups.insert(make_pair(last_part_name, 1));
      part_name = last_part_name + "_1";
    } else {
      ++dup_it->second;
      char buf[512];
      snprintf(buf, sizeof(buf), "%s_%d", last_part_name.GetCString(), dup_it->second);
      part_name.Set(buf);
    }
    if (names.count(part_name) > 0) error("Failed to patch mesh part name to %s.\n", part_name.GetCString());
  };
  String last_part_name;
  for (u32 i = 0; i < scene->mRootNode->mNumChildren; ++i) {
    const aiNode* child = scene->mRootNode->mChildren[i];
    for (u32 mi = 0; mi < child->mNumMeshes; ++mi) {
      parts.push_back(MeshPart());
      MeshPart& part = parts.back();
      aiMesh* sub_mesh = scene->mMeshes[child->mMeshes[mi]];
      if (sub_mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
        printf("Ignore non-triangles: %s.\n", sub_mesh->mName.C_Str());
        parts.resize(parts.size() - 1);
        continue;
      }
      String part_name(child->mName.C_Str());
      if (g_options.input_type == INPUT_TYPE_OBJ && part_name.StartsWith("g ")) part_name = part_name.Substr(2);
      patch_part_name(part_names, dup_part_names, part_name, last_part_name);
      last_part_name = part_name;
      part_names.insert(part_name);
      part.name = part_name.GetID();
      if (scene->HasMaterials()) {
        aiMaterial* material = scene->mMaterials[sub_mesh->mMaterialIndex];
        aiString material_name;
        if (aiGetMaterialString(material, AI_MATKEY_NAME, &material_name) == aiReturn_SUCCESS) {
          part.material = String::GetID(material_name.C_Str());
          material_names.insert(material_name.C_Str());
        } else {
          part.material = 0;
        }
      } else part.material = 0;
      part.start_index = num_indices;
      part.num_indices = sub_mesh->mNumFaces * 3;
      u32 vstart_offset = num_vertices * decl.stride;
      num_vertices += sub_mesh->mNumVertices;
      vertex_buf = (u8*)realloc(vertex_buf, num_vertices * decl.stride);
      copy_vertex_attr(vertex_buf + vstart_offset,
                       sub_mesh->mNumVertices, decl.stride,
                       sub_mesh->mVertices, sizeof(float) * 3);
      part.aabb.SetFrom((vec*)sub_mesh->mVertices, sub_mesh->mNumVertices);
      header.aabb.Enclose(part.aabb);
      if (sub_mesh->HasNormals()) {
        copy_vertex_attr(vertex_buf + vstart_offset + decl.offsets[VERTEX_ATTR_NORMAL],
                         sub_mesh->mNumVertices, decl.stride,
                         sub_mesh->mNormals, sizeof(float) * 3);
      }
      if (sub_mesh->HasTextureCoords(0)) {
        copy_vertex_attr(vertex_buf + vstart_offset + decl.offsets[VERTEX_ATTR_TEXCOORD0],
                         sub_mesh->mNumVertices, decl.stride,
                         sub_mesh->mTextureCoords[0], sizeof(float) * 2);
      }
      if (sub_mesh->HasTextureCoords(1)) {
        copy_vertex_attr(vertex_buf + vstart_offset + decl.offsets[VERTEX_ATTR_TEXCOORD1],
                         sub_mesh->mNumVertices, decl.stride,
                         sub_mesh->mTextureCoords[1], sizeof(float) * 2);
      }
      if (sub_mesh->HasVertexColors(0)) {
        copy_vertex_attr(vertex_buf + vstart_offset + decl.offsets[VERTEX_ATTR_COLOR0],
                         sub_mesh->mNumVertices, decl.stride,
                         sub_mesh->mColors[0], sizeof(float) * 4);
      }
      if (sub_mesh->HasVertexColors(1)) {
        copy_vertex_attr(vertex_buf + vstart_offset + decl.offsets[VERTEX_ATTR_COLOR1],
                         sub_mesh->mNumVertices, decl.stride,
                         sub_mesh->mColors[1], sizeof(float) * 4);
      }

      if (!sub_mesh->HasFaces()) error("Mesh part '%s' doesn't have faces.", sub_mesh->mName.C_Str());

      u32 istart_offset = index_size * num_indices;
      num_indices += sub_mesh->mNumFaces * 3;
      if (index_size == 2 && num_vertices >= 0x10000) {
        u8* tmp_buf = (u8*)index_buf;
        index_buf = (u8*)malloc(4 * num_indices);
        for (u32 i = 0; i < istart_offset / 2; ++i) {
          *((u32*)index_buf + i) = *((u16*)tmp_buf + i);
        }
        free(tmp_buf);
        index_size = 4;
        istart_offset *= 2;
      } else index_buf = (u8*)realloc(index_buf, index_size * num_indices);
      for (u32 j = 0; j < sub_mesh->mNumFaces; ++j) {
        aiFace* face = sub_mesh->mFaces + j;
        if (face->mNumIndices != 3) error("Expect every face to have 3 indices.");
        u32 start_vertex = vstart_offset / decl.stride;
        if (index_size == 2) {
          u16* index = (u16*)(index_buf + istart_offset) + j * 3;
          *index++ = face->mIndices[0] + start_vertex;
          *index++ = face->mIndices[1] + start_vertex;
          *index++ = face->mIndices[2] + start_vertex;
        } else {
          u32* index = (u32*)(index_buf + istart_offset) + j * 3;
          *index++ = face->mIndices[0] + start_vertex;
          *index++ = face->mIndices[1] + start_vertex;
          *index++ = face->mIndices[2] + start_vertex;
        }
      }
    }
  }
  header.num_parts = (u16)parts.size();
  header.num_vertices = num_vertices;
  header.num_indices = num_indices;

  FILE* f = fopen(g_options.output_filename.GetCString(), "wb");
  if (!f) error("Failed to open output file '%s'.", g_options.output_filename.GetCString());
  fseek(f, sizeof(header), SEEK_SET);
  u32 strings_size = 0;
  for (auto& name : part_names) strings_size += name.GetLength() + 1;
  for (auto& name : material_names) strings_size += name.GetLength() + 1;
  fwrite(&strings_size, sizeof(u32), 1, f);
  for (auto& name : part_names) fwrite(name.GetCString(), name.GetLength() + 1, 1, f);
  for (auto& name : material_names) fwrite(name.GetCString(), name.GetLength() + 1, 1, f);

  u8 zeros[16];
  memset(zeros, 0, sizeof(zeros));

  u32 file_offset = (u32)ftell(f);
  header.part_data_offset = ALIGN_16(file_offset);
  fwrite(zeros, 1, header.part_data_offset - file_offset, f);
  fwrite(parts.data(), sizeof(MeshPart), parts.size(), f);

  file_offset = (u32)ftell(f);
  header.vertex_data_offset = ALIGN_16(file_offset);
  fwrite(zeros, 1, header.vertex_data_offset - file_offset, f);
  fwrite(vertex_buf, decl.stride, num_vertices, f);

  file_offset = (u32)ftell(f);
  header.index_data_offset = ALIGN_16(file_offset);
  fwrite(zeros, 1, header.index_data_offset - file_offset, f);
  fwrite(index_buf, index_size, num_indices, f);

  fseek(f, 0, SEEK_SET);
  fwrite(&header, sizeof(header), 1, f);
  fclose(f);
  if (vertex_buf) free(vertex_buf);
  if (index_buf) free(index_buf);

  if (g_options.verbose) {
    auto find_string = [](const unordered_set<String>& table, stringid name) -> const char* {
      for (auto& s : table) if (s.GetID() == name) return s.GetCString();
      return "";
    };

    printf("Vertices: %d, Tris: %d, Parts: %d, %s\n", header.num_vertices,
           header.num_indices / 3, header.num_parts, header.aabb.ToString().c_str());
    for (auto& p : parts) {
      printf("%16s: Tris: %d, Material: %s\n", find_string(part_names, p.name),
             p.num_indices / 3, find_string(material_names, p.material));
    }
    decl.Dump();
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2) exit(-1);

  g_options.input_filename.Set(argv[1]);
  g_options.output_filename = g_options.input_filename.MakeWithAnotherSuffix(".mex");
  g_options.model_name = g_options.output_filename;
  g_options.model_name.RemoveSuffix();
  g_options.input_type = g_options.input_filename.GetLastSection('.').MakeLower().GetID();
  g_options.attr_mask = (1 << VERTEX_ATTR_POSITION);
  g_options.verbose = 1;
  int idx = max(g_options.model_name.FindLast('/'), g_options.model_name.FindLast('\\'));
  if (idx != -1) g_options.model_name = g_options.model_name.Substr(idx + 1);

  Assimp::Importer importer;
  unsigned int flags = aiProcess_CalcTangentSpace |
    aiProcess_Triangulate |
    aiProcess_JoinIdenticalVertices |
    aiProcess_OptimizeMeshes |
    aiProcess_ImproveCacheLocality |
    aiProcess_SortByPType;
  if (g_options.input_type == INPUT_TYPE_FBX) flags |= aiProcess_PreTransformVertices;
  auto scene = importer.ReadFile(argv[1], flags);
  if (!scene) {
    error("Failed to read file %s.", argv[1]);
    exit(-1);
  }

  process(scene);
  system("pause");
  return 0;
}
