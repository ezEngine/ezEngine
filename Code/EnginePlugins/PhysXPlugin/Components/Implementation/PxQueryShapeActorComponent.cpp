#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <PhysXPlugin/Components/PxQueryShapeActorComponent.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>

using namespace physx;

ezPxQueryShapeActorComponentManager::ezPxQueryShapeActorComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezPxQueryShapeActorComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezPxQueryShapeActorComponentManager::~ezPxQueryShapeActorComponentManager() = default;

void ezPxQueryShapeActorComponentManager::UpdateKinematicActors()
{
  EZ_PROFILE_SCOPE("QueryShapeActors");

  for (auto pKinematicActorComponent : m_KinematicActorComponents)
  {
    if (PxRigidDynamic* pActor = pKinematicActorComponent->GetPxActor())
    {
      ezGameObject* pObject = pKinematicActorComponent->GetOwner();

      pObject->UpdateGlobalTransform();

      const ezVec3 pos = pObject->GetGlobalPosition();
      const ezQuat rot = pObject->GetGlobalRotation();

      PxTransform t = ezPxConversionUtils::ToTransform(pos, rot);
      pActor->setGlobalPose(t);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxQueryShapeActorComponent, 1, ezComponentMode::Static)
EZ_END_COMPONENT_TYPE
// clang-format on

ezPxQueryShapeActorComponent::ezPxQueryShapeActorComponent() = default;
ezPxQueryShapeActorComponent::~ezPxQueryShapeActorComponent() = default;

void ezPxQueryShapeActorComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  // auto& s = stream.GetStream();
}

void ezPxQueryShapeActorComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // auto& s = stream.GetStream();
}

void ezPxQueryShapeActorComponent::OnSimulationStarted()
{
  if (!IsActive())
    return;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  const ezSimdTransform& globalTransform = GetOwner()->GetGlobalTransformSimd();

  PxTransform t = ezPxConversionUtils::ToTransform(globalTransform);
  m_pActor = ezPhysX::GetSingleton()->GetPhysXAPI()->createRigidDynamic(t);
  EZ_ASSERT_DEBUG(m_pActor != nullptr, "PhysX actor creation failed");

  ezPxUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);
  m_pActor->userData = pUserData;

  // PhysX does not get any scale value, so to correctly position child objects
  // we have to pretend that this parent object applies no scale on its children
  ezSimdTransform globalTransformNoScale = globalTransform;
  globalTransformNoScale.m_Scale.Set(1.0f);
  AddShapesFromObject(GetOwner(), m_pActor, globalTransformNoScale);

  m_pActor->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, true);

  // if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<ezPxQueryShapeActorComponentManager>()->m_KinematicActorComponents.PushBack(this);
  }

  {
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
    pModule->GetPxScene()->addActor(*m_pActor);
  }
}

void ezPxQueryShapeActorComponent::OnDeactivated()
{
  // if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<ezPxQueryShapeActorComponentManager>()->m_KinematicActorComponents.RemoveAndSwap(this);
  }

  if (m_pActor)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>();

    if (pModule->GetPxScene())
    {
      EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));

      m_pActor->release();
    }

    m_pActor = nullptr;

    pModule->DeallocateUserData(m_uiUserDataIndex);
  }

  SUPER::OnDeactivated();
}
