#pragma once

#include "Memory/MemoryMacros.h"

#define DEFINE_SINGLETON_FUNCTIONS_WITHOUT_CREATOR(ClassName) \
  static ClassName* Instance() { return __instance; } \
  static void ReleaseInstance() { safe_delete(__instance); }

#define DEFINE_SINGLETON_FUNCTIONS(ClassName) \
  static ClassName* CreateInstance() { safe_delete(__instance); return __instance = new ClassName; } \
  DEFINE_SINGLETON_FUNCTIONS_WITHOUT_CREATOR(ClassName)

#define DEFINE_SINGLETON_FUNCTIONS_WITH_ONE_ARG_CREATOR(ClassName, ArgType) \
  static ClassName* CreateInstance(ArgType arg) { safe_delete(__instance); return __instance = new ClassName(arg); } \
  DEFINE_SINGLETON_FUNCTIONS_WITHOUT_CREATOR(ClassName)

#define DECLARE_SINGLETON_INSTANCE(ClassName) \
  static ClassName* __instance;

#define SUPPORT_SINGLETON(ClassName) \
  public: \
    DEFINE_SINGLETON_FUNCTIONS(ClassName) \
  private: \
    DECLARE_SINGLETON_INSTANCE(ClassName)

#define SUPPORT_SINGLETON_WITH_ONE_ARG_CREATOR(ClassName, ArgType) \
  public: \
    DEFINE_SINGLETON_FUNCTIONS_WITH_ONE_ARG_CREATOR(ClassName, ArgType) \
  private: \
    DECLARE_SINGLETON_INSTANCE(ClassName)


#define DEFINE_SINGLETON_INSTANCE(ClassName) \
  ClassName* ClassName::__instance = nullptr;
