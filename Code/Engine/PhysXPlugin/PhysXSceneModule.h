#pragma once

#include <PhysXPlugin/Basics.h>
#include <PhysXPlugin/PluginInterface.h>
#include <Foundation/Configuration/Plugin.h>
#include <Core/Scene/SceneModule.h>

class ezPxErrorCallback : public PxErrorCallback
{
public:
  virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override;
};

class ezPxAllocatorCallback : public PxAllocatorCallback
{
public:
  ezPxAllocatorCallback();

  virtual void* allocate(size_t size, const char* typeName, const char* filename, int line) override;
  virtual void deallocate(void* ptr) override;

  ezProxyAllocator m_Allocator;
};

class EZ_PHYSXPLUGIN_DLL ezPhysX : public ezPhysXInterface
{
public:
  ezPhysX();

  static ezPhysX* GetSingleton();

  void Startup();
  void Shutdown();

  virtual PxPhysics* GetPhysXAPI() override { return m_pPhysX; }

  virtual PxMaterial* GetDefaultMaterial() const { return m_pDefaultMaterial; }

private:
  void StartupVDB();
  void ShutdownVDB();

  bool m_bInitialized;
  PxFoundation* m_pFoundation;
  ezPxErrorCallback m_ErrorCallback;
  ezPxAllocatorCallback m_AllocatorCallback;
  PxProfileZoneManager* m_pProfileZoneManager;
  PxPhysics* m_pPhysX;
  PxMaterial* m_pDefaultMaterial;
  PxVisualDebuggerConnection* m_VdbConnection;
};


class EZ_PHYSXPLUGIN_DLL ezPhysXSceneModule : public ezSceneModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysXSceneModule, ezSceneModule);

public:
  ezPhysXSceneModule();

  PxScene* GetPxScene() const { return m_pPxScene; }


protected:
  virtual void InternalStartup() override;
  virtual void InternalShutdown() override;
  virtual void InternalUpdate() override;

private:
  PxScene* m_pPxScene;
  PxCpuDispatcher* m_pCPUDispatcher;

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(PhysX, PhysXPlugin);
};


EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_PHYSXPLUGIN_DLL, ezPhysXPlugin);