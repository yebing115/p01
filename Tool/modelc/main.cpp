#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/material.h>
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "Data/DataType.h"
#include "Data/String.h"
#include "Data/Json.h"
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
  String output_dir;
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

void copy_vertex_attr(u8* buf, u32 num, u16 stride, const void* attr_data,
                      size_t elem_size, size_t attr_stride = 0) {
  const u8* src = (const u8*)attr_data;
  for (u32 i = 0; i < num; ++i) {
    memcpy(buf, src, elem_size);
    buf += stride;
    if (attr_stride) src += attr_stride;
    else src += elem_size;
  }
}

const char* get_basename(const char* str) {
  const char* p = max<const char*>(strrchr(str, '/'), strrchr(str, '\\'));
  return p ? ++p : str;
}

void process_material(aiMaterial* material, const char* filename) {
  bool has_specular_tex = (material->GetTextureCount(aiTextureType_SPECULAR) > 0);
  bool has_normal_tex = (material->GetTextureCount(aiTextureType_NORMALS) > 0);
  bool has_opacity_tex = (material->GetTextureCount(aiTextureType_OPACITY) > 0);

  u32 buf_capacity = 256 << 10;
  char* buf = (char*)malloc(buf_capacity);
  JsonWriter writer(buf, buf_capacity);

  const char* flags_string = "";
  aiString path;
  aiTextureMapMode mapmode = aiTextureMapMode_Wrap;
  
  writer.BeginWriteObject();
  if (has_specular_tex && has_normal_tex) writer.WriteString("shader", "model_specular_normal");
  else if (has_specular_tex) writer.WriteString("shader", "model_specular");
  else if (has_normal_tex) writer.WriteString("shader", "model_normal");
  else writer.WriteString("shader", "model_diffuse");
  
  writer.BeginWriteObject("properties");

  {
    writer.BeginWriteObject("diffuse_tex");
    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr,
                             nullptr, nullptr, &mapmode) != aiReturn_SUCCESS) {
      path = "WHITE";
    }
    if (mapmode == aiTextureMapMode_Clamp) flags_string = "UV_CLAMP";
    else if (mapmode == aiTextureMapMode_Mirror) flags_string = "UV_MIRROR";
    writer.WriteString("value", get_basename(path.C_Str()));
    writer.WriteString("flags", flags_string);
    writer.EndWriteObject();
  }

  if (has_opacity_tex) {
    writer.BeginWriteObject("opacity_tex");
    material->GetTexture(aiTextureType_OPACITY, 0, &path, nullptr, nullptr,
                         nullptr, nullptr, &mapmode);
    if (mapmode == aiTextureMapMode_Clamp) flags_string = "UV_CLAMP";
    else if (mapmode == aiTextureMapMode_Mirror) flags_string = "UV_MIRROR";
    writer.WriteString("value", get_basename(path.C_Str()));
    writer.WriteString("flags", flags_string);
    writer.EndWriteObject();
  }

  if (has_specular_tex) {
    writer.BeginWriteObject("specular_tex");
    material->GetTexture(aiTextureType_SPECULAR, 0, &path, nullptr, nullptr,
                         nullptr, nullptr, &mapmode);
    if (mapmode == aiTextureMapMode_Clamp) flags_string = "UV_CLAMP";
    else if (mapmode == aiTextureMapMode_Mirror) flags_string = "UV_MIRROR";
    writer.WriteString("value", get_basename(path.C_Str()));
    writer.WriteString("flags", flags_string);
    writer.EndWriteObject();
  }

  if (has_normal_tex) {
    writer.BeginWriteObject("normal_tex");
    material->GetTexture(aiTextureType_NORMALS, 0, &path, nullptr, nullptr,
                         nullptr, nullptr, &mapmode);
    if (mapmode == aiTextureMapMode_Clamp) flags_string = "UV_CLAMP";
    else if (mapmode == aiTextureMapMode_Mirror) flags_string = "UV_MIRROR";
    writer.WriteString("value", get_basename(path.C_Str()));
    writer.WriteString("flags", flags_string);
    writer.EndWriteObject();
  }

  writer.EndWriteObject(); // properties
  writer.EndWriteObject(); // root
  if (!writer.IsValid()) error("Failed to import material '%s'.\n", filename);

  char fullpath[256];
  strcpy(fullpath, g_options.output_dir.GetCString());
  strcat(fullpath, "/");
  strcat(fullpath, filename);
  FILE* f = fopen(fullpath, "wb");
  if (!f) error("Failed to open: '%s'\n", fullpath);
  fwrite(buf, 1, strlen(buf), f);
  fclose(f);
  free(buf);
}

void process(const aiScene* scene) {
  MeshHeader header;
  AABB model_aabb;
  memset(&header, 0, sizeof(header));
  header.magic = C3_CHUNK_MAGIC_MEX;
  model_aabb.SetNegativeInfinity();
  vector<MeshPart> parts;
  VertexDecl decl;
  unordered_set<String> part_names;
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
  if (first_mesh->HasTangentsAndBitangents()) {
    decl.Add(VERTEX_ATTR_TANGENT, 3, FLOAT_TYPE);
    attr->attr = VERTEX_ATTR_TANGENT;
    attr->num = 3;
    attr->data_type = FLOAT_TYPE;
    attr->flags = MESH_ATTR_DEFAULT;
    ++attr;
  }
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

  header.num_materials = scene->mNumMaterials;
  vector<MeshMaterial> materials(header.num_materials);
  for (u32 i = 0; i < scene->mNumMaterials; ++i) {
    auto& mesh_material = materials[i];
    memset(&mesh_material, 0, sizeof(mesh_material));
    auto mat = scene->mMaterials[i];
    aiString name;
    if (mat->Get(AI_MATKEY_NAME, name) == aiReturn_SUCCESS) {
      sprintf(mesh_material.filename, "%s_%s.mat", g_options.model_name.GetCString(),
              name.C_Str());
      process_material(mat, mesh_material.filename);
    }
  }

  u8* vertex_buf = nullptr;
  u32 num_vertices = 0;
  u8* index_buf = nullptr;
  u32 num_indices = 0;
  u32 index_size = 2;
  // Ensure every part has different name, rename and append suffix '_1', '_2' ... if needed.
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
      strcpy(part.name, part_name.GetCString());
      part.material_index = sub_mesh->mMaterialIndex;
      part.start_index = num_indices;
      part.num_indices = sub_mesh->mNumFaces * 3;
      u32 vstart_offset = num_vertices * decl.stride;
      num_vertices += sub_mesh->mNumVertices;
      vertex_buf = (u8*)realloc(vertex_buf, num_vertices * decl.stride);
      copy_vertex_attr(vertex_buf + vstart_offset,
                       sub_mesh->mNumVertices, decl.stride,
                       sub_mesh->mVertices, sizeof(float) * 3);
      AABB part_aabb;
      part_aabb.SetFrom((vec*)sub_mesh->mVertices, sub_mesh->mNumVertices);
      model_aabb.Enclose(part_aabb);
      part.aabb_min = part_aabb.minPoint;
      part.aabb_max = part_aabb.maxPoint;
      if (sub_mesh->HasNormals()) {
        copy_vertex_attr(vertex_buf + vstart_offset + decl.offsets[VERTEX_ATTR_NORMAL],
                         sub_mesh->mNumVertices, decl.stride,
                         sub_mesh->mNormals, sizeof(float) * 3);
      }
      if (sub_mesh->HasTangentsAndBitangents()) {
        copy_vertex_attr(vertex_buf + vstart_offset + decl.offsets[VERTEX_ATTR_TANGENT],
                         sub_mesh->mNumVertices, decl.stride,
                         sub_mesh->mTangents, sizeof(float) * 3);
      }
      if (sub_mesh->HasTextureCoords(0)) {
        copy_vertex_attr(vertex_buf + vstart_offset + decl.offsets[VERTEX_ATTR_TEXCOORD0],
                         sub_mesh->mNumVertices, decl.stride,
                         sub_mesh->mTextureCoords[0], sizeof(float) * 2, sizeof(float) * 3);
      }
      if (sub_mesh->HasTextureCoords(1)) {
        copy_vertex_attr(vertex_buf + vstart_offset + decl.offsets[VERTEX_ATTR_TEXCOORD1],
                         sub_mesh->mNumVertices, decl.stride,
                         sub_mesh->mTextureCoords[1], sizeof(float) * 2, sizeof(float) * 3);
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
  header.aabb_min = model_aabb.minPoint;
  header.aabb_max = model_aabb.maxPoint;
  header.num_parts = (u16)parts.size();
  header.num_vertices = num_vertices;
  header.num_indices = num_indices;

  FILE* f = fopen(g_options.output_filename.GetCString(), "wb");
  if (!f) error("Failed to open output file '%s'.", g_options.output_filename.GetCString());
  fseek(f, sizeof(header), SEEK_SET);

  u32 file_offset;
  u8 zeros[16];
  memset(zeros, 0, sizeof(zeros));

  file_offset = (u32)ftell(f);
  header.material_data_offset = ALIGN_16(file_offset);
  fwrite(zeros, 1, header.material_data_offset - file_offset, f);
  fwrite(materials.data(), sizeof(MeshMaterial), materials.size(), f);

  file_offset = (u32)ftell(f);
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
    printf("Materials:\n");
    for (int i = 0; i < materials.size(); ++i) {
      printf("  %d: %s\n", i, materials[i].filename);
    }
    printf("Vertices: %d, Tris: %d, Parts: %d, %s\n", header.num_vertices,
           header.num_indices / 3, header.num_parts, model_aabb.ToString().c_str());
    for (auto& p : parts) {
      printf("%16s: Tris: %d, Material: %d\n", p.name,
             p.num_indices / 3, p.material_index);
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
  g_options.output_dir = ".";
  int idx = max(g_options.model_name.FindLast('/'), g_options.model_name.FindLast('\\'));
  if (idx != -1) g_options.model_name = g_options.model_name.Substr(idx + 1);
  idx = max(g_options.output_filename.FindLast('/'),
                g_options.output_filename.FindLast('\\'));
  if (idx != -1) g_options.output_dir = g_options.output_filename.Substr(0, idx);

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
