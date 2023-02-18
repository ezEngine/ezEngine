#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <PhysXPlugin/PhysXInterface.h>

#include <PxPhysicsAPI.h>
using namespace physx;

class ezPxErrorCallback : public PxErrorCallback
{
public:
  virtual void reportError(PxErrorCode::Enum code, const char* szMessage, const char* szFile, int iLine) override;
};

class ezPxAllocatorCallback : public PxAllocatorCallback
{
public:
  ezPxAllocatorCallback();

  virtual void* allocate(size_t uiSize, const char* szTypeName, const char* szFilename, int iLine) override;
  virtual void deallocate(void* pPtr) override;

  void VerifyAllocations();

  ezProxyAllocator m_Allocator;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezHashTable<void*, ezString> m_Allocations;
#endif
};

class ezPxQueryFilter : public PxQueryFilterCallback
{
public:
  /// \brief Whether shapes that are only used for scene queries (not for simulation) should be checked.
  bool m_bIncludeQueryShapes = true;

  virtual PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* pShape, const PxRigidActor* pActor, PxHitFlags& ref_queryFlags) override;
  virtual PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override;
};


//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPhysX : public ezPhysXInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezPhysX, ezPhysXInterface);

public:
  ezPhysX();
  ~ezPhysX();

  void Startup();
  void Shutdown();

  void StartupVDB();
  void ShutdownVDB();

  virtual PxPhysics* GetPhysXAPI() override { return m_pPhysX; }

  PxMaterial* GetDefaultMaterial() const { return m_pDefaultMaterial; }

  virtual ezCollisionFilterConfig& GetCollisionFilterConfig() override { return m_CollisionFilterConfig; }

  virtual void LoadCollisionFilters() override;


  ezAllocatorBase* GetAllocator();


  // helper functions

  static PxFilterData CreateFilterData(ezUInt32 uiCollisionLayer, ezUInt32 uiShapeId = ezInvalidIndex, ezBitflags<ezOnPhysXContact> flags = ezOnPhysXContact::None);

private:
  void SurfaceResourceEventHandler(const ezSurfaceResourceEvent& e);

  bool m_bInitialized;
  PxFoundation* m_pFoundation;
  ezPxErrorCallback m_ErrorCallback;
  ezPxAllocatorCallback* m_pAllocatorCallback;
  PxPhysics* m_pPhysX;
  PxMaterial* m_pDefaultMaterial;
  PxPvd* m_pPvdConnection;
  ezCollisionFilterConfig m_CollisionFilterConfig;
};
