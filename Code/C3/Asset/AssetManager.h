#pragma once
#include "Graphics/Model/Model.h"
#include "Graphics/Material/Program.h"
#include "Graphics/Material/Texture.h"

enum AssetType {
};

enum AssetState {
};

struct AssetHeader;
struct Asset {
  AssetState _state;
  AssetHeader* _header;
};

struct AssetHeader {
  u32 _type;
  u32 _size;
  u32 _ref;
  AssetHeader* _next;
  AssetHeader* _prev;
  union {
    Texture _texture;
    Model _model;
    Program _program;
    u8 _p;
  };
}

class AssetManager {
public:
  AssetManager();
  ~AssetManager();

  AssetHeader* Get(stringid filename);
  AssetHeader* Load(stringid filename);
  void Prefetch(stringid filename);
  void Unload(Asset* asset);
  bool IsLoaded(stringid filename);
  static AssetDesc Resolve(stringid filename);

private:
  RefObject* _Load(const AssetDescription& desc);
  RefObject* _Get(const AssetDescription& desc);
  void _Unload(const AssetDescription& desc);
  void NextTask();
  bool UpdateTask();
  void AddTask(const AssetDescription& desc);
  void IncrementDependencies(const AssetDescription& parent_filename);
  void AddAsset(const AssetDescription& desc, RefObject* resource);
  void LoadDependencies(const AssetDescription& desc, const vector<AssetDescription>& depends);
  void LoadDependency(const AssetDescription& depend_desc);
  JOB_HANDLE Submit(ThreadFn fn, void* data);
  bool CheckJobFinished(JOB_HANDLE handle, int32* status = nullptr);

  unordered_map<AssetDescription, RefObject*> _resources;
  unordered_map<String, LoaderFactory*> _loader_factories;
  unordered_map<AssetDescription, vector<AssetDescription>> _dependencies;
  std::list<AssetDescription> _loading_queue;
  vector<LoadingTaskRef> _tasks;
  unordered_set<String> _depends_set;
  uint32 _to_load;
  uint32 _loaded;
  LoadingThead _thread;

  friend class LoadingTask;

  SUPPORT_SINGLETON(AssetManager);
};