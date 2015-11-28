#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/PhysXSceneModule.h>
#include <Core/World/World.h>

#include <PhysXPlugin/Components/PxStaticActorComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Components/PxShapeBoxComponent.h>
#include <PhysXPlugin/Components/PxShapeSphereComponent.h>
#include <PhysXPlugin/Components/PxShapeCapsuleComponent.h>
#include <PhysXPlugin/Components/PxCenterOfMassComponent.h>
#include <PhysXPlugin/Joints/PxDistanceJointComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysXSceneModule, 1, ezRTTIDefaultAllocator<ezPhysXSceneModule>);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPhysXSceneModule::ezPhysXSceneModule()
{
}

void ezPhysXSceneModule::InternalStartup()
{
  GetWorld()->CreateComponentManager<ezPxStaticActorComponentManager>()->SetUserData(this);
  GetWorld()->CreateComponentManager<ezPxDynamicActorComponentManager>()->SetUserData(this);
  GetWorld()->CreateComponentManager<ezPxShapeBoxComponentManager>()->SetUserData(this);
  GetWorld()->CreateComponentManager<ezPxShapeSphereComponentManager>()->SetUserData(this);
  GetWorld()->CreateComponentManager<ezPxShapeCapsuleComponentManager>()->SetUserData(this);
  GetWorld()->CreateComponentManager<ezPxCenterOfMassComponentManager>()->SetUserData(this);
  GetWorld()->CreateComponentManager<ezPxDistanceJointComponentManager>()->SetUserData(this);

  PxSceneDesc desc = PxSceneDesc(PxTolerancesScale());
  desc.setToDefault(PxTolerancesScale());

  desc.gravity = PxVec3(0.0f, 0.0f, -9.81f);

  m_pCPUDispatcher = PxDefaultCpuDispatcherCreate(4);
  desc.cpuDispatcher = m_pCPUDispatcher;
  desc.filterShader = PxDefaultSimulationFilterShader;

  EZ_ASSERT_DEV(desc.isValid(), "PhysX scene description is invalid");
  m_pPxScene = ezPhysX::GetSingleton()->GetPhysXAPI()->createScene(desc);

  EZ_ASSERT_ALWAYS(m_pPxScene != nullptr, "Creating the PhysX scene failed");
}

void ezPhysXSceneModule::InternalShutdown()
{
  m_pPxScene->release();
  m_pPxScene = nullptr;
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


