#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Utilities/PxUserData.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPxSteppingMode, 1)
  EZ_ENUM_CONSTANTS(ezPxSteppingMode::Variable, ezPxSteppingMode::Fixed, ezPxSteppingMode::SemiFixed)
EZ_END_STATIC_REFLECTED_ENUM();

//////////////////////////////////////////////////////////////////////////

void ezPxErrorCallback::reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
{
  switch (code)
  {
  case PxErrorCode::eABORT:
    ezLog::Error("PhysX: {0}", message);
    break;
  case PxErrorCode::eDEBUG_INFO:
    ezLog::Dev("PhysX: {0}", message);
    break;
  case PxErrorCode::eDEBUG_WARNING:
    ezLog::Warning("PhysX: {0}", message);
    break;
  case PxErrorCode::eINTERNAL_ERROR:
    ezLog::Error("PhysX Internal: {0}", message);
    break;
  case PxErrorCode::eINVALID_OPERATION:
    EZ_REPORT_FAILURE("PhysX Invalid Operation: {0}", message);
    break;
  case PxErrorCode::eINVALID_PARAMETER:
    EZ_REPORT_FAILURE("PhysX Invalid Parameter: {0}", message);
    break;
  case PxErrorCode::eOUT_OF_MEMORY:
    ezLog::Error("PhysX Out-of-Memory: {0}", message);
    break;
  case PxErrorCode::ePERF_WARNING:
    ezLog::Warning("PhysX Performance: {0}", message);
    break;

  default:
    ezLog::Error("PhysX: Unknown error type '{0}': {1}", code, message);
    break;
  }
}

//////////////////////////////////////////////////////////////////////////

ezPxAllocatorCallback::ezPxAllocatorCallback()
  : m_Allocator("PhysX", ezFoundation::GetAlignedAllocator())
{

}

void* ezPxAllocatorCallback::allocate(size_t size, const char* typeName, const char* filename, int line)
{
  //return new unsigned char[size];
  void* pPtr = m_Allocator.Allocate(size, 16);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezStringBuilder s;
  s.Set(typeName, " - ", filename);
  m_Allocations[pPtr] = s;
#endif

  return pPtr;
}

void ezPxAllocatorCallback::deallocate(void* ptr)
{
  // apparently this happens
  if (ptr == nullptr)
    return;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  m_Allocations.Remove(ptr);
#endif

  m_Allocator.Deallocate(ptr);
}

void ezPxAllocatorCallback::VerifyAllocations()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  EZ_ASSERT_DEV(m_Allocations.IsEmpty(), "There are {0} unfreed allocations", m_Allocations.GetCount());

  for (auto it = m_Allocations.GetIterator(); it.IsValid(); ++it)
  {
    const char* s = it.Value().GetData();
    ezLog::Info(s);
  }
#endif
}

//////////////////////////////////////////////////////////////////////////

PxQueryHitType::Enum ezPxQueryFilter::preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags)
{
  const PxFilterData& shapeFilterData = shape->getQueryFilterData();

  queryFlags = (PxHitFlags)0;

  // shape should be ignored
  if (shapeFilterData.word2 == filterData.word2)
  {
    return PxQueryHitType::eNONE;
  }

  // trigger the contact callback for pairs (A,B) where
  // the filter mask of A contains the ID of B and vice versa.
  if ((filterData.word0 & shapeFilterData.word1) || (shapeFilterData.word0 & filterData.word1))
  {
    queryFlags |= PxHitFlag::eDEFAULT;
    return PxQueryHitType::eBLOCK;
  }

  return PxQueryHitType::eNONE;
}

PxQueryHitType::Enum ezPxQueryFilter::postFilter(const PxFilterData& filterData, const PxQueryHit& hit)
{
  return PxQueryHitType::eNONE;
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SINGLETON(ezPhysX);
static ezPhysX g_PhysXSingleton;

ezPhysX::ezPhysX()
  : m_SingletonRegistrar(this)
{
  m_bInitialized = false;

  m_pFoundation = nullptr;
  m_pAllocatorCallback = nullptr;
  m_pProfileZoneManager = nullptr;
  m_pPhysX = nullptr;
  m_pDefaultMaterial = nullptr;
  m_VdbConnection = nullptr;
}

ezPhysX::~ezPhysX()
{

}

void ezPhysX::Startup()
{
  if (m_bInitialized)
    return;

  m_bInitialized = true;

  m_pAllocatorCallback = EZ_DEFAULT_NEW(ezPxAllocatorCallback);

  m_pFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, *m_pAllocatorCallback, m_ErrorCallback);
  EZ_ASSERT_DEV(m_pFoundation != nullptr, "Initializing PhysX failed");

  bool bRecordMemoryAllocations = false;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  bRecordMemoryAllocations = true;
#endif

  /// \todo This seems to be in the Extensions library, which does not compile with VS 2015
  m_pProfileZoneManager = nullptr;// &PxProfileZoneManager::createProfileZoneManager(m_pFoundation);
                                  //EZ_ASSERT_DEV(m_pProfileZoneManager != nullptr, "Initializing PhysX Profile Zone Manager failed");

  m_pPhysX = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, PxTolerancesScale(), bRecordMemoryAllocations, m_pProfileZoneManager);
  EZ_ASSERT_DEV(m_pPhysX != nullptr, "Initializing PhysX API failed");

  m_pDefaultMaterial = m_pPhysX->createMaterial(0.6f, 0.4f, 0.25f);

  ezSurfaceResource::s_Events.AddEventHandler(ezMakeDelegate(&ezPhysX::SurfaceResourceEventHandler, this));

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  StartupVDB();
#endif
}

void ezPhysX::Shutdown()
{
  if (!m_bInitialized)
    return;

  m_bInitialized = false;

  ShutdownVDB();

  ezSurfaceResource::s_Events.RemoveEventHandler(ezMakeDelegate(&ezPhysX::SurfaceResourceEventHandler, this));

  if (m_pDefaultMaterial != nullptr)
  {
    m_pDefaultMaterial->release();
    m_pDefaultMaterial = nullptr;
  }

  if (m_pPhysX != nullptr)
  {
    m_pPhysX->release();
    m_pPhysX = nullptr;
  }

  if (m_pProfileZoneManager != nullptr)
  {
    m_pProfileZoneManager->release();
    m_pProfileZoneManager = nullptr;
  }

  if (m_pFoundation != nullptr)
  {
    m_pFoundation->release();
    m_pFoundation = nullptr;
  }

  m_pAllocatorCallback->VerifyAllocations();
  EZ_DEFAULT_DELETE(m_pAllocatorCallback);
}

void ezPhysX::StartupVDB()
{
  // check if PvdConnection manager is available on this platform
  if (m_pPhysX->getPvdConnectionManager() == nullptr)
    return;

  // return if we already have a connection
  if (m_VdbConnection != nullptr)
    return;

  // setup connection parameters
  const char* pvd_host_ip = "127.0.0.1"; // IP of the PC which is running PVD
  int port = 5425; // TCP port to connect to, where PVD is listening
  unsigned int timeout = 100; // timeout in milliseconds to wait for PVD to respond, consoles and remote PCs need a higher timeout.

  PxVisualDebuggerConnectionFlags connectionFlags = PxVisualDebuggerExt::getAllConnectionFlags();

  m_VdbConnection = PxVisualDebuggerExt::createConnection(m_pPhysX->getPvdConnectionManager(), pvd_host_ip, port, timeout, connectionFlags);

  m_pPhysX->getVisualDebugger()->setVisualDebuggerFlags(PxVisualDebuggerFlag::eTRANSMIT_CONSTRAINTS | PxVisualDebuggerFlag::eTRANSMIT_CONTACTS | PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES);
}

void ezPhysX::ShutdownVDB()
{
  if (m_VdbConnection == nullptr)
    return;

  m_VdbConnection->release();
  m_VdbConnection = nullptr;
}

void ezPhysX::LoadCollisionFilters()
{
  EZ_LOG_BLOCK("ezPhysX::LoadCollisionFilters");

  if (m_CollisionFilterConfig.Load("Physics/CollisionLayers.cfg").Failed())
  {
    ezLog::Info("Collision filter config file could not be found ('Physics/CollisionLayers.cfg'). Using default values.");

    // setup some default config

    m_CollisionFilterConfig.SetGroupName(0, "Default");
    m_CollisionFilterConfig.EnableCollision(0, 0);
  }
}

//static
void ezPhysX::addForceAtPos(PxRigidBody& body, const PxVec3& force, const PxVec3& globalPos, PxForceMode::Enum mode)
{
  const PxTransform globalPose = body.getGlobalPose();
  const PxVec3 centerOfMass = globalPose.transform(body.getCMassLocalPose().p);

  const PxVec3 torque = (globalPos - centerOfMass).cross(force);
  body.addForce(force, mode);
  body.addTorque(torque, mode);
}


PxFilterData ezPhysX::CreateFilterData(ezUInt32 uiCollisionLayer, ezUInt32 uiShapeId, bool bReportContact)
{
  PxFilterData filter;
  filter.word0 = EZ_BIT(uiCollisionLayer);
  filter.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(uiCollisionLayer);
  filter.word2 = uiShapeId;
  filter.word3 = bReportContact ? 1 : 0;

  return filter;
}

void ezPhysX::SurfaceResourceEventHandler(const ezSurfaceResource::Event& e)
{
  if (e.m_Type == ezSurfaceResource::Event::Type::Created)
  {
    const auto& desc = e.m_pSurface->GetDescriptor();

    PxMaterial* pMaterial = m_pPhysX->createMaterial(desc.m_fPhysicsFrictionStatic, desc.m_fPhysicsFrictionDynamic, desc.m_fPhysicsRestitution);
    pMaterial->userData = EZ_DEFAULT_NEW(ezPxUserData, e.m_pSurface);

    e.m_pSurface->m_pPhysicsMaterial = pMaterial;
  }
  else if (e.m_Type == ezSurfaceResource::Event::Type::Destroyed)
  {
    if (e.m_pSurface->m_pPhysicsMaterial != nullptr)
    {
      PxMaterial* pMaterial = static_cast<PxMaterial*>(e.m_pSurface->m_pPhysicsMaterial);

      ezPxUserData* pUserData = static_cast<ezPxUserData*>(pMaterial->userData);
      EZ_DEFAULT_DELETE(pUserData);

      pMaterial->release();

      e.m_pSurface->m_pPhysicsMaterial = nullptr;
    }
  }
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_WorldModule_Implementation_PhysX);

