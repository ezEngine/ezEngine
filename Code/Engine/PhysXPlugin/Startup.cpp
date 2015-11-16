#include <PhysXPlugin/PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <PhysXPlugin/PhysXSceneModule.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(PhysX, PhysXPlugin)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
}

ON_CORE_SHUTDOWN
{
}

ON_ENGINE_STARTUP
{
  ezPhysXSceneModule::InitializePhysX();
}

ON_ENGINE_SHUTDOWN
{
  ezPhysXSceneModule::DeinitializePhysX();
}

EZ_END_SUBSYSTEM_DECLARATION

void ezPxErrorCallback::reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
{
  switch (code)
  {
  case PxErrorCode::eABORT:
    ezLog::Error("PhysX: %s", message);
    break;
  case PxErrorCode::eDEBUG_INFO:
    ezLog::Dev("PhysX: %s", message);
    break;
  case PxErrorCode::eDEBUG_WARNING:
    ezLog::Warning("PhysX: %s", message);
    break;
  case PxErrorCode::eINTERNAL_ERROR:
    ezLog::Error("PhysX Internal: %s", message);
    break;
  case PxErrorCode::eINVALID_OPERATION:
    ezLog::Error("PhysX Invalid Operation: %s", message);
    break;
  case PxErrorCode::eINVALID_PARAMETER:
    ezLog::Error("PhysX Invalid Parameter: %s", message);
    break;
  case PxErrorCode::eOUT_OF_MEMORY:
    ezLog::Error("PhysX Out-of-Memory: %s", message);
    break;
  case PxErrorCode::ePERF_WARNING:
    ezLog::Warning("PhysX Performance: %s", message);
    break;

  default:
    ezLog::Error("PhysX: Unknown error type '%i': %s", code, message);
    break;
  }
}

ezPxAllocatorCallback::ezPxAllocatorCallback()
  : m_Allocator("PhysX", ezFoundation::GetDefaultAllocator())
{

}

void* ezPxAllocatorCallback::allocate(size_t size, const char* typeName, const char* filename, int line)
{
  return m_Allocator.Allocate(size, 16);
}

void ezPxAllocatorCallback::deallocate(void* ptr)
{
  // apparently this happens
  if (ptr == nullptr)
    return;

  m_Allocator.Deallocate(ptr);
}

ezPhysXData* ezPhysXSceneModule::s_pPhysXData = nullptr;

void ezPhysXSceneModule::InitializePhysX()
{
  EZ_ASSERT_DEV(s_pPhysXData == nullptr, "Data must not be initialized already");

  s_pPhysXData = EZ_DEFAULT_NEW(ezPhysXData);

  s_pPhysXData->m_pFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, s_pPhysXData->m_AllocatorCallback, s_pPhysXData->m_ErrorCallback);

  EZ_ASSERT_DEV(s_pPhysXData->m_pFoundation != nullptr, "Initializing PhysX failed");


  bool bRecordMemoryAllocations = false;
#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  bRecordMemoryAllocations = true;
#  endif

  /// \todo This seems to be in the Extensions library, which does not compile with VS 2015
  s_pPhysXData->m_pProfileZoneManager = nullptr;// &PxProfileZoneManager::createProfileZoneManager(s_pPhysXData->m_pFoundation);

  //EZ_ASSERT_DEV(s_pPhysXData->m_pProfileZoneManager != nullptr, "Initializing PhysX Profile Zone Manager failed");

  s_pPhysXData->m_pPhysX = PxCreatePhysics(PX_PHYSICS_VERSION, *s_pPhysXData->m_pFoundation, PxTolerancesScale(), bRecordMemoryAllocations, s_pPhysXData->m_pProfileZoneManager);

  EZ_ASSERT_DEV(s_pPhysXData->m_pPhysX != nullptr, "Initializing PhysX API failed");


  s_pPhysXData->m_pDefaultMaterial = s_pPhysXData->m_pPhysX->createMaterial(0.6f, 0.4f, 0.25f);
}

void ezPhysXSceneModule::DeinitializePhysX()
{
  if (s_pPhysXData != nullptr)
  {
    if (s_pPhysXData->m_pPhysX != nullptr)
      s_pPhysXData->m_pPhysX->release();

    if (s_pPhysXData->m_pFoundation != nullptr)
      s_pPhysXData->m_pFoundation->release();

    EZ_DEFAULT_DELETE(s_pPhysXData);
  }
}

