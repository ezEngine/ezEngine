#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/PluginInterface.h>
#include <PhysXPlugin/PhysXWorldModule.h>

EZ_IMPLEMENT_SINGLETON(ezPhysX);

static ezPhysX g_PhysXSingleton;

ezPhysX::ezPhysX()
  : m_SingletonRegistrar(this)
{
  m_bInitialized = false;

  m_pFoundation = nullptr;
  m_pProfileZoneManager = nullptr;
  m_pPhysX = nullptr;
  m_pDefaultMaterial = nullptr;
  m_VdbConnection = nullptr;
}

void ezPhysX::LoadCollisionFilters()
{
  EZ_LOG_BLOCK("ezPhysX::LoadCollisionFilters");

  if (m_CollisionFilterConfig.Load("Physics/CollisionLayers.cfg").Failed())
  {
    ezLog::InfoPrintf("Collision filter config file could not be found ('Physics/CollisionLayers.cfg'). Using default values.");

    // setup some default config

    m_CollisionFilterConfig.SetGroupName(0, "Default");
    m_CollisionFilterConfig.EnableCollision(0, 0);
  }
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
#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  bRecordMemoryAllocations = true;
#  endif

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



void ezPhysX::SurfaceResourceEventHandler(const ezSurfaceResource::Event& e)
{
  if (e.m_Type == ezSurfaceResource::Event::Type::Created)
  {
    const auto& desc = e.m_pSurface->GetDescriptor();

    PxMaterial* pMaterial = m_pPhysX->createMaterial(desc.m_fPhysicsFrictionStatic, desc.m_fPhysicsFrictionDynamic, desc.m_fPhysicsRestitution);
    pMaterial->userData = e.m_pSurface;

    e.m_pSurface->m_pPhysicsMaterial = pMaterial;
  }
  else if (e.m_Type == ezSurfaceResource::Event::Type::Destroyed)
  {
    if (e.m_pSurface->m_pPhysicsMaterial != nullptr)
    {
      PxMaterial* pMaterial = static_cast<PxMaterial*>(e.m_pSurface->m_pPhysicsMaterial);

      pMaterial->release();

      e.m_pSurface->m_pPhysicsMaterial = nullptr;
    }
  }

}
