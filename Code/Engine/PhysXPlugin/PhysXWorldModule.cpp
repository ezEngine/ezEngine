#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/PhysXWorldModule.h>
#include <Core/World/World.h>

#include <PhysXPlugin/Components/PxStaticActorComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Shapes/PxShapeBoxComponent.h>
#include <PhysXPlugin/Shapes/PxShapeSphereComponent.h>
#include <PhysXPlugin/Shapes/PxShapeCapsuleComponent.h>
#include <PhysXPlugin/Shapes/PxShapeConvexComponent.h>
#include <PhysXPlugin/Components/PxCenterOfMassComponent.h>
#include <PhysXPlugin/Joints/PxDistanceJointComponent.h>
#include <PhysXPlugin/Joints/PxFixedJointComponent.h>
#include <PhysXPlugin/Components/PxCharacterControllerComponent.h>
#include <PhysXPlugin/Components/PxSettingsComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysXWorldModule, 1, ezRTTIDefaultAllocator<ezPhysXWorldModule>);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

PxFilterFlags ezPxFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
  // let triggers through
  if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
  {
    pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
    return PxFilterFlag::eDEFAULT;
  }

  pairFlags = (PxPairFlag::Enum)0;

  // trigger the contact callback for pairs (A,B) where
  // the filter mask of A contains the ID of B and vice versa.
  if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
  {
    pairFlags |= PxPairFlag::eCONTACT_DEFAULT;
    return PxFilterFlag::eDEFAULT;
  }

  return PxFilterFlag::eKILL;
}


ezPhysXWorldModule::ezPhysXWorldModule()
{
  m_pPxScene = nullptr;

  SetGravity(ezVec3(0, 0, -10), ezVec3(0, 0, -12));
}

void ezPhysXWorldModule::SetGravity(const ezVec3& objectGravity, const ezVec3& characterGravity)
{
  m_vObjectGravity = objectGravity;
  m_vCharacterGravity = characterGravity;

  if (m_pPxScene)
  {
    m_pPxScene->setGravity(PxVec3(m_vObjectGravity.x, m_vObjectGravity.y, m_vObjectGravity.z));
  }
}

void ezPhysXWorldModule::InternalStartup()
{
  InternalReinit();

  GetWorld()->GetOrCreateComponentManager<ezPxStaticActorComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxDynamicActorComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxShapeBoxComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxShapeSphereComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxShapeCapsuleComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxShapeConvexComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxCenterOfMassComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxDistanceJointComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxFixedJointComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxCharacterControllerComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxSettingsComponentManager>()->SetUserData(this);

  m_AccumulatedTimeSinceUpdate.SetZero();

  PxSceneDesc desc = PxSceneDesc(PxTolerancesScale());
  desc.setToDefault(PxTolerancesScale());

  desc.gravity = PxVec3(m_vObjectGravity.x, m_vObjectGravity.y, m_vObjectGravity.z); /// \todo Scene settings

  m_pCPUDispatcher = PxDefaultCpuDispatcherCreate(4);
  desc.cpuDispatcher = m_pCPUDispatcher;
  desc.filterShader = ezPxFilterShader;

  EZ_ASSERT_DEV(desc.isValid(), "PhysX scene description is invalid");
  m_pPxScene = ezPhysX::GetSingleton()->GetPhysXAPI()->createScene(desc);

  m_pCharacterManager = PxCreateControllerManager(*m_pPxScene);

  EZ_ASSERT_ALWAYS(m_pPxScene != nullptr, "Creating the PhysX scene failed");
}

void ezPhysXWorldModule::InternalShutdown()
{
  //m_pCharacterManager->purgeControllers();
  m_pCharacterManager->release();
  m_pCharacterManager = nullptr;

  m_pPxScene->release();
  m_pPxScene = nullptr;

  m_pCPUDispatcher->release();
  m_pCPUDispatcher = nullptr;
}

void ezPhysXWorldModule::InternalUpdate()
{
  if (!GetWorld()->GetWorldSimulationEnabled())
    return;

  EZ_LOCK(GetWorld()->GetWriteMarker());

  if (ezPxSettingsComponent* pSettings = GetWorld()->GetOrCreateComponentManager<ezPxSettingsComponentManager>()->GetSingletonComponent())
  {
    if (pSettings->IsModified())
    {
      SetGravity(pSettings->GetObjectGravity(), pSettings->GetCharacterGravity());

      pSettings->ResetModified();
    }
  }

  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  m_AccumulatedTimeSinceUpdate += tDiff;

  const ezTime tStep = ezTime::Seconds(1.0 / 60.0);

  while (m_AccumulatedTimeSinceUpdate >= tStep)
  {
    m_pPxScene->simulate((PxReal) tStep.GetSeconds());
    m_pPxScene->fetchResults(true);

    m_AccumulatedTimeSinceUpdate -= tStep;
  }

  // not sure where exactly this needs to go
  m_pCharacterManager->computeInteractions((float)tDiff.GetSeconds());
}


void ezPhysXWorldModule::InternalReinit()
{
  ezPhysX::GetSingleton()->LoadCollisionFilters();

}

void ezPxAllocatorCallback::VerifyAllocations()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  EZ_ASSERT_DEV(m_Allocations.IsEmpty(), "There are %u unfreed allocations", m_Allocations.GetCount());

  for (auto it = m_Allocations.GetIterator(); it.IsValid(); ++it)
  {
    const char* s = it.Value().GetData();
    ezLog::Info(s);
  }
#endif
}

