#pragma once

#include <PhysXPlugin/Basics.h>
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


struct ezPhysXData
{
  PxFoundation* m_pFoundation;
  ezPxErrorCallback m_ErrorCallback;
  ezPxAllocatorCallback m_AllocatorCallback;
  PxProfileZoneManager* m_pProfileZoneManager;
  PxPhysics* m_pPhysX;
  PxMaterial* m_pDefaultMaterial;
};


class EZ_PHYSXPLUGIN_DLL ezPhysXSceneModule : public ezSceneModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysXSceneModule, ezSceneModule);

public:
  ezPhysXSceneModule();

  PxScene* GetPxScene() const { return m_pPxScene; }

  PxPhysics* GetPxApi() const { return s_pPhysXData->m_pPhysX; }

  PxMaterial* GetDefaultMaterial() const { return s_pPhysXData->m_pDefaultMaterial; }

protected:
  virtual void InternalStartup() override;

  virtual void InternalShutdown() override;

  virtual void InternalUpdate() override;

private:
  PxScene* m_pPxScene;
  PxCpuDispatcher* m_pCPUDispatcher;

private:
  static void InitializePhysX();
  static void DeinitializePhysX();

  static ezPhysXData* s_pPhysXData;

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(PhysX, PhysXPlugin);
};


EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_PHYSXPLUGIN_DLL, ezPhysXPlugin);