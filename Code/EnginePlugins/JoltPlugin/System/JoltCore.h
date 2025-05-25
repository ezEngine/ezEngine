#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <GameEngine/Physics/WeightCategory.h>
#include <JoltPlugin/JoltPluginDLL.h>

class ezJoltMaterial;
struct ezSurfaceResourceEvent;
class ezJoltDebugRenderer;
class ezWorld;

namespace JPH
{
  class JobSystem;
}

class EZ_JOLTPLUGIN_DLL ezJoltCore
{
public:
  static JPH::JobSystem* GetJoltJobSystem() { return s_pJobSystem.Borrow(); }
  static const ezJoltMaterial* GetDefaultMaterial() { return s_pDefaultMaterial; }

  static void DebugDraw(ezWorld* pWorld);

#ifdef JPH_DEBUG_RENDERER
  static std::unique_ptr<ezJoltDebugRenderer> s_pDebugRenderer;
#endif

  static const ezCollisionFilterConfig& GetCollisionFilterConfig();
  static const ezWeightCategoryConfig& GetWeightCategoryConfig();

  static void ReloadConfigs();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Jolt, JoltPlugin);

  static void Startup();
  static void Shutdown();

  static void SurfaceResourceEventHandler(const ezSurfaceResourceEvent& e);

  static void* JoltMalloc(size_t inSize);
  static void JoltFree(void* inBlock);
  static void* JoltReallocate(void* inBlock, size_t inOldSize, size_t inNewSize);
  static void* JoltAlignedMalloc(size_t inSize, size_t inAlignment);
  static void JoltAlignedFree(void* inBlock);

  static void LoadCollisionFilters();
  static void LoadWeightCategories();

  static ezJoltMaterial* s_pDefaultMaterial;
  static ezUniquePtr<JPH::JobSystem> s_pJobSystem;

  static ezUniquePtr<ezProxyAllocator> s_pAllocator;
  static ezUniquePtr<ezProxyAllocator> s_pAllocatorAligned;

  static ezCollisionFilterConfig s_CollisionFilterConfig;
  static ezWeightCategoryConfig s_WeightCategoryConfig;
};
