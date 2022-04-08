#pragma once

#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Types/UniquePtr.h>
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
  static ezUniquePtr<ezJoltDebugRenderer> s_pDebugRenderer;
#endif

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Jolt, JoltPlugin);

  static void Startup();
  static void Shutdown();

  static void SurfaceResourceEventHandler(const ezSurfaceResourceEvent& e);

  static ezJoltMaterial* s_pDefaultMaterial;
  static ezUniquePtr<JPH::JobSystem> s_pJobSystem;
};
