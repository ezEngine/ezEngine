#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/PhysXSceneModule.h>
#include <Core/World/World.h>

#include <PhysXPlugin/Components/StaticMeshComponent.h>
#include <PhysXPlugin/Components/RigidBodyComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysXSceneModule, 1, ezRTTIDefaultAllocator<ezPhysXSceneModule>);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();


ezPhysXSceneModule::ezPhysXSceneModule()
{


}

void ezPhysXSceneModule::InternalStartup()
{
  GetWorld()->CreateComponentManager<ezPxStaticMeshComponentManager>()->SetUserData(this);
  GetWorld()->CreateComponentManager<ezPxRigidBodyComponentManager>()->SetUserData(this);

  PxSceneDesc desc = PxSceneDesc(PxTolerancesScale());
  desc.setToDefault(PxTolerancesScale());

  desc.gravity = PxVec3(0.0f, 0.0f, -9.81f);

  m_pCPUDispatcher = PxDefaultCpuDispatcherCreate(4);
  desc.cpuDispatcher = m_pCPUDispatcher;
  desc.filterShader = PxDefaultSimulationFilterShader;

  EZ_ASSERT_DEV(desc.isValid(), "PhysX scene description is invalid");
  m_pPxScene = s_pPhysXData->m_pPhysX->createScene(desc);

  EZ_ASSERT_ALWAYS(m_pPxScene != nullptr, "Creating the PhysX scene failed");
}

void ezPhysXSceneModule::InternalShutdown()
{
}

void ezPhysXSceneModule::InternalUpdate()
{
  if (!GetWorld()->GetWorldSimulationEnabled())
    return;

  ezTime tDiff = ezClock::Get(ezGlobalClock_GameLogic)->GetTimeDiff();

  const ezTime tStep = ezTime::Seconds(1.0 / 60.0);

  while (tDiff >= tStep)
  {
    m_pPxScene->simulate((PxReal) tStep.GetSeconds());
    m_pPxScene->fetchResults(true);

    tDiff -= tStep;
  }
}
