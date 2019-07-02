#pragma once

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <PhysXPlugin/PhysXInterface.h>

#include <GameEngine/Physics/CollisionFilter.h>
#include <GameEngine/Physics/SurfaceResource.h>
#include <PxPhysicsAPI.h>
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
  virtual PxQueryHitType::Enum preFilter(
    const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override;
  virtual PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override;
};


//////////////////////////////////////////////////////////////////////////

struct ezPhysXFilterFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = 0,
    ContactReports = EZ_BIT(0),
    SurfaceInteractions = EZ_BIT(1),

    Default = None
  };

  struct Bits
  {
    StorageType ContactReports : 1;
    StorageType SurfaceInteractions : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezPhysXFilterFlags);

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

  static PxFilterData CreateFilterData(ezUInt32 uiCollisionLayer, ezUInt32 uiShapeId = ezInvalidIndex, ezBitflags<ezPhysXFilterFlags> flags = ezPhysXFilterFlags::None);

private:
  void SurfaceResourceEventHandler(const ezSurfaceResource::Event& e);

  bool m_bInitialized;
  PxFoundation* m_pFoundation;
  ezPxErrorCallback m_ErrorCallback;
  ezPxAllocatorCallback* m_pAllocatorCallback;
  PxPhysics* m_pPhysX;
  PxMaterial* m_pDefaultMaterial;
  PxPvd* m_PvdConnection;
  ezCollisionFilterConfig m_CollisionFilterConfig;
};
