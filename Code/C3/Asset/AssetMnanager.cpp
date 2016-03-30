#include "AssetManager.h"

DEFINE_SINGLETON_INSTANCE(AssetManager);

AssetManager::AssetManager(): _to_load(0), _loaded(0) {
  _thread.Init();

  SetLoader(".dds", new DDSLoaderFactory);
  auto almighty = new AlmightyLoaderFactory;
  SetLoader(".png", almighty);
  SetLoader(".jpg", almighty);
  SetLoader(".jpeg", almighty);
  SetLoader(".tga", almighty);
#if ON_PS4
  SetLoader(".gnf", new GNFLoaderFactory);
#endif

  SetLoader(".obj", new ObjModelLoaderFactory);
  SetLoader(".json", new SpineModelLoaderFactory);
}

AssetManager::~AssetManager() {
  Clear();
  for (auto& p : _loader_factories) safe_delete(p.second);
  _loader_factories.clear();
}

Ref<RefObject> AssetManager::Find(const String& filename) {
  auto it = _resources.find(filename);
  if (it != _resources.end()) return Ref<RefObject>(it->second);
  return nullptr;
}

RefObject* AssetManager::_Load(const AssetDescription& desc) {
  auto obj = _Get(desc);
  if (obj) {
    obj->Increment();
    IncrementDependencies(desc);
    return obj;
  }
  auto loader = GetLoader(desc.filename);
  if (!loader) return nullptr;
  if (loader->DependenciesGuess()) {
    auto deps = loader->GetDependencies(desc);
    if (!deps.empty()) {
      for (auto& dep : deps) {
        _Load(dep);
      }
      _dependencies[desc] = deps;
    }
  }
  auto resource = loader->Load(desc);
  if (resource) {
    resource->Increment();
    IncrementDependencies(desc);
    _resources[desc] = resource;
  }
  PutLoader(loader);
  return resource;
}

RefObject* AssetManager::_Get(const AssetDescription& desc) {
  auto it = _resources.find(desc);
  return (it == _resources.end()) ? nullptr : it->second;
}

void AssetManager::_Unload(const AssetDescription& desc) {
  if (!_tasks.empty() && _tasks.back()->_desc == desc) {
    _tasks.back()->_cancel = true;
    return;
  }
  if (!_loading_queue.empty()) {
    bool removed = false;
    for (auto it = _loading_queue.begin(), end = _loading_queue.end(); it != end; ++it) {
      if (*it == desc) {
        removed = true;
        _loading_queue.erase(it);
        break;
      }
    }
    if (removed) return;
  }
  auto it = _resources.find(desc);
  if (it != _resources.end()) {
    auto obj = it->second;
    obj->Decrement();
    if (obj->Unreferenced()) {
      _resources.erase(it);
      delete obj;
    }
  }
  auto dep_it = _dependencies.find(desc);
  if (dep_it != _dependencies.end()) {
    for (auto& dep : dep_it->second) {
      if (_Get(dep)) _Unload(dep);
    }
  }
}

void AssetManager::LoadAsync(const String& filename) {
  if (_loading_queue.empty()) _to_load = _loaded = 0;
  ++_to_load;
  _loading_queue.push_back(Resolve(filename));
}

void AssetManager::Unload(const String& filename) {
  _Unload(Resolve(filename));
}

bool AssetManager::IsLoaded(const String& filename) {
  auto desc = Resolve(filename);
  return (_resources.find(desc) != _resources.end());
}

void AssetManager::Clear() {
  _loading_queue.clear();
  while (!Update(0))
    ;
  _to_load = _loaded = 0;
}

float AssetManager::GetProgress() const {
  return _to_load == 0 ? 1.f : min(1.f, (float)_loaded / _to_load);
}

Loader* AssetManager::GetLoader(const String& filename) {
  auto suffix = filename.GetSuffixName().ToLower();
  auto it = _loader_factories.find(suffix);
  if (it == _loader_factories.end()) return nullptr;
  auto loader = it->second->CreateLoader();
  loader->factory = it->second;
  return loader;
}

void AssetManager::PutLoader(Loader* loader) {
  loader->factory->DestroyLoader(loader);
}

void AssetManager::SetLoader(const String& suffix, LoaderFactory* factory) {
  _loader_factories[suffix] = factory;
}

bool AssetManager::Update(float dt) {
  if (_tasks.empty()) {
    if (!_loading_queue.empty()) {
      while (!_loading_queue.empty() && _tasks.empty()) NextTask();
      if (_tasks.empty()) return true;
    } else return true;
  }
  return UpdateTask() && _loading_queue.empty() && _tasks.empty();
}

AssetDescription AssetManager::Resolve(const String& filename) {
  static const stringid PNG_STRINGID = hash_string(".png");
  static const stringid JPG_STRINGID = hash_string(".jpg");
  static const stringid DDS_STRINGID = hash_string(".dds");
  static const stringid GNF_STRINGID = hash_string(".gnf");
  static const stringid SKIN_STRINGID = hash_string(".skin");

  AssetDescription desc(filename);
  auto suffix_id = filename.GetSuffixName().ToLower().GetID();
  if (suffix_id == PNG_STRINGID || suffix_id == JPG_STRINGID ||
      suffix_id == DDS_STRINGID || suffix_id == GNF_STRINGID) {
    uint32 flags = C2_TEXTURE_MIN_ANISOTROPIC | C2_TEXTURE_MAG_ANISOTROPIC | C2_TEXTURE_U_CLAMP | C2_TEXTURE_V_CLAMP;
    if (AppConfig::USE_SRGB) flags |= C2_TEXTURE_SRGB;
    desc.param = flags;

    if (suffix_id == PNG_STRINGID || suffix_id == JPG_STRINGID) {
#if ON_WINDOWS
      desc.filename.ChangeSuffix(".dds");
#elif ON_PS4
      desc.filename.ChangeSuffix(".gnf");
#endif
      if (!FileSystem::Instance()->Exists(desc.filename)) {
        c2_log("[C2] Warn: '%s' not exists.\n", desc.filename.GetCString());
        desc.filename = filename;
      }
    }
  } else if (suffix_id == SKIN_STRINGID) {
    desc.filename.RemoveLastSection();
    auto skin_name = get_basename(filename);
    skin_name.RemoveSuffix();
    desc.param = skin_name.GetID();
  }
  return desc;
}

void AssetManager::NextTask() {
  auto& desc = _loading_queue.front();
  auto resource = _Get(desc.filename);
  if (resource) {
    resource->Increment();
    IncrementDependencies(desc.filename);
    ++_loaded;
  } else {
    AddTask(desc);
  }
  _loading_queue.pop_front();
}

bool AssetManager::UpdateTask() {
  auto task = _tasks.back();

  bool complete = task->_cancel || task->Update();
  if (complete) {
    _tasks.pop_back();
    if (_tasks.empty()) ++_loaded;
    PutLoader(task->_loader);

    if (task->_cancel || !task->_resource) return true;

    AddAsset(task->_desc, task->_resource);
    
    auto end_time = get_timestamp();
    c2_log("Loaded: %s, time = %.3lf ms\n", task->_desc.filename.GetCString(), (end_time - task->_start_time) * 1000.0);

    return true;
  }
  return false;
}

void AssetManager::AddTask(const AssetDescription& desc) {
  auto loader = GetLoader(desc.filename);
  if (!loader) {
    c2_log("Failed to find loader for '%s'\n", desc.filename.GetCString());
    return;
  }
  _tasks.push_back(make_shared<LoadingTask>(this, loader, desc));
}

void AssetManager::IncrementDependencies(const AssetDescription& parent_desc) {
  auto it = _dependencies.find(parent_desc);
  if (it != _dependencies.end()) {
    for (auto& dep : it->second) {
      auto r = _Get(dep);
      if (r) {
        r->Increment();
        IncrementDependencies(dep);
      }
    }
  }
}

void AssetManager::AddAsset(const AssetDescription& desc, RefObject* resource) {
  auto loaded = _Get(desc);
  if (loaded) {
    loaded->Increment();
    delete resource;
    return;
  }
  resource->Increment();
  _resources[desc] = resource;
}

void AssetManager::LoadDependencies(const AssetDescription& desc, const vector<AssetDescription>& depends) {
  if (depends.empty()) return;

  for (auto& desc : depends) {
    //c2_log("depends: %s -> %s\n", desc.filename.GetCString(), filename.GetCString());
    LoadDependency(desc);
  }
  _dependencies[desc] = depends;
}

void AssetManager::LoadDependency(const AssetDescription& depend_desc) {
  auto r = _Get(depend_desc.filename);
  if (r) {
    r->Increment();
    IncrementDependencies(depend_desc.filename);
  } else {
    AddTask(depend_desc);
  }
}

JOB_HANDLE AssetManager::Submit(ThreadFn fn, void* data) {
  return _thread.AddJob(fn, data);
}

bool AssetManager::CheckJobFinished(JOB_HANDLE handle, int32* status) {
  bool finished = _thread.GetJobStatus(handle, status);
  if (finished) _thread.RemoveJob(handle);
  return finished;
}
