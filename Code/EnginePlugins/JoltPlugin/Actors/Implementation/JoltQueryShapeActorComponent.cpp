#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

ezJoltQueryShapeActorComponentManager::ezJoltQueryShapeActorComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezJoltQueryShapeActorComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezJoltQueryShapeActorComponentManager::~ezJoltQueryShapeActorComponentManager() = default;

void ezJoltQueryShapeActorComponentManager::UpdateMovingQueryShapes()
{
  EZ_PROFILE_SCOPE("UpdateMovingQueryShapes");

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  for (auto pComponent : m_MovingQueryShapes)
  {
    JPH::BodyID bodyId(pComponent->m_uiJoltBodyID);

    if (bodyId.IsInvalid())
      continue;

    ezGameObject* pObject = pComponent->GetOwner();

    pObject->UpdateGlobalTransform();

    const ezSimdVec4f pos = pObject->GetGlobalPositionSimd();
    const ezSimdQuat rot = pObject->GetGlobalRotationSimd();

    pBodies->SetPositionAndRotation(bodyId, ezJoltConversionUtils::ToVec3(pos), ezJoltConversionUtils::ToQuat(rot), JPH::EActivation::DontActivate);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltQueryShapeActorComponent, 1, ezComponentMode::Static)
EZ_END_COMPONENT_TYPE
// clang-format on

ezJoltQueryShapeActorComponent::ezJoltQueryShapeActorComponent() = default;
ezJoltQueryShapeActorComponent::~ezJoltQueryShapeActorComponent() = default;

void ezJoltQueryShapeActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  const ezSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  JPH::BodyCreationSettings bodyCfg;

  if (CreateShape(&bodyCfg, 1.0f).Failed())
  {
    ezLog::Error("Jolt query-shape actor component '{}' has no valid shape.", GetOwner()->GetName());
    return;
  }

  ezJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  bodyCfg.mPosition = ezJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = ezJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType = JPH::EMotionType::Static;
  bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::QueryShape);
  bodyCfg.mMotionQuality = JPH::EMotionQuality::Discrete;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mUserData = reinterpret_cast<ezUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody);

  if (GetOwner()->IsDynamic())
  {
    GetWorld()->GetOrCreateComponentManager<ezJoltQueryShapeActorComponentManager>()->m_MovingQueryShapes.PushBack(this);
  }
}

void ezJoltQueryShapeActorComponent::OnDeactivated()
{
  if (GetOwner()->IsDynamic())
  {
    GetWorld()->GetOrCreateComponentManager<ezJoltQueryShapeActorComponentManager>()->m_MovingQueryShapes.RemoveAndSwap(this);
  }

  SUPER::OnDeactivated();
}
