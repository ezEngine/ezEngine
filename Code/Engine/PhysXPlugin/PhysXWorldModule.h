#pragma once

#include <PhysXPlugin/Basics.h>
#include <PhysXPlugin/PluginInterface.h>
#include <Foundation/Configuration/Plugin.h>
#include <Core/World/WorldModule.h>
#include <GameUtils/Surfaces/SurfaceResource.h>
#include <GameUtils/CollisionFilter/CollisionFilter.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>

#include <PxPhysicsAPI.h>
#include <Foundation/Configuration/Singleton.h>
using namespace physx;

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

  void VerifyAllocations();

  ezProxyAllocator m_Allocator;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezHashTable<void*, ezString> m_Allocations;
#endif
};

class ezPxQueryFilter : public PxQueryFilterCallback
{
public:
  virtual PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
  {
    queryFlags = (PxHitFlags)0;

    // trigger the contact callback for pairs (A,B) where
    // the filter mask of A contains the ID of B and vice versa.
    if ((filterData.word0 & shape->getQueryFilterData().word1) || (shape->getQueryFilterData().word0 & filterData.word1))
    {
      queryFlags |= PxHitFlag::eDEFAULT;
      return PxQueryHitType::eBLOCK;
    }

    return PxQueryHitType::eNONE;
  }

  virtual PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override
  {
    return PxQueryHitType::eNONE;
  }
};

class EZ_PHYSXPLUGIN_DLL ezPhysX : public ezPhysXInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezPhysX, ezPhysXInterface);

public:
  ezPhysX();

  void Startup();
  void Shutdown();

  virtual PxPhysics* GetPhysXAPI() override { return m_pPhysX; }

  PxMaterial* GetDefaultMaterial() const { return m_pDefaultMaterial; }

  virtual ezCollisionFilterConfig& GetCollisionFilterConfig() override { return m_CollisionFilterConfig; }

  virtual void LoadCollisionFilters() override;

private:
  void StartupVDB();
  void ShutdownVDB();

  void SurfaceResourceEventHandler(const ezSurfaceResource::Event& e);

  bool m_bInitialized;
  PxFoundation* m_pFoundation;
  ezPxErrorCallback m_ErrorCallback;
  ezPxAllocatorCallback* m_pAllocatorCallback;
  PxProfileZoneManager* m_pProfileZoneManager;
  PxPhysics* m_pPhysX;
  PxMaterial* m_pDefaultMaterial;
  PxVisualDebuggerConnection* m_VdbConnection;
  ezCollisionFilterConfig m_CollisionFilterConfig;
  
};


class EZ_PHYSXPLUGIN_DLL ezPhysXWorldModule : public ezPhysicsWorldModuleInterface
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysXWorldModule, ezPhysicsWorldModuleInterface);

public:
  ezPhysXWorldModule();

  PxScene* GetPxScene() const { return m_pPxScene; }

  virtual ezVec3 GetGravity() const override { return m_vObjectGravity; }
  ezVec3 GetCharacterGravity() const { return m_vCharacterGravity; }

  void SetGravity(const ezVec3& objectGravity, const ezVec3& characterGravity);

  PxControllerManager* GetCharacterManager() const { return m_pCharacterManager; }

  virtual bool CastRay(const ezVec3& vStart, const ezVec3& vDir, float fMaxLen, ezUInt8 uiCollisionLayer, ezVec3& out_vHitPos, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitGameObject, ezSurfaceResourceHandle& out_hSurface) override;

  virtual bool SweepTestCapsule(const ezTransform& start, const ezVec3& vDir, float fCapsuleRadius, float fCapsuleHeight, float fDistance, ezUInt8 uiCollisionLayer, float& out_fDistance, ezVec3& out_Position, ezVec3& out_Normal) override;

protected:
  virtual void InternalStartup() override;
  virtual void InternalShutdown() override;
  virtual void InternalUpdate() override;
  virtual void InternalReinit() override;

private:
  PxScene* m_pPxScene;
  PxDefaultCpuDispatcher* m_pCPUDispatcher;
  ezTime m_AccumulatedTimeSinceUpdate;
  PxControllerManager* m_pCharacterManager;
  ezVec3 m_vObjectGravity;
  ezVec3 m_vCharacterGravity;

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(PhysX, PhysXPlugin);
};


EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_PHYSXPLUGIN_DLL, ezPhysXPlugin);