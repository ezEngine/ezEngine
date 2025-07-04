#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
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

    JPH::Quat jRot = ezJoltConversionUtils::ToQuat(rot);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    // Jolt is overly strict about normalization
    jRot = jRot.Normalized();
#endif

    pBodies->SetPositionAndRotation(bodyId, ezJoltConversionUtils::ToVec3(pos), jRot, JPH::EActivation::DontActivate);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltQueryShapeActorComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezJoltQueryShapeActorComponent::ezJoltQueryShapeActorComponent() = default;
ezJoltQueryShapeActorComponent::~ezJoltQueryShapeActorComponent() = default;

void ezJoltQueryShapeActorComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hSurface;
}

void ezJoltQueryShapeActorComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hSurface;
}

void ezJoltQueryShapeActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  const ezSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  JPH::BodyCreationSettings bodyCfg;

  if (CreateShape(&bodyCfg, 1.0f, GetJoltMaterial()).Failed())
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
  bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Query);
  bodyCfg.mMotionQuality = JPH::EMotionQuality::Discrete;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  // bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // the group filter is only needed for objects constrained via joints
  bodyCfg.mUserData = reinterpret_cast<ezUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  EZ_ASSERT_DEV(pBody != nullptr, "Jolt body creation failed. You need to increase the maximum number of bodies.");

  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, false);

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

void ezJoltQueryShapeActorComponent::SetSurfaceFile(ezStringView sFile)
{
  if (!sFile.IsEmpty())
  {
    m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(sFile);
  }
  else
  {
    m_hSurface = {};
  }

  if (m_hSurface.IsValid())
    ezResourceManager::PreloadResource(m_hSurface);
}

ezStringView ezJoltQueryShapeActorComponent::GetSurfaceFile() const
{
  return m_hSurface.GetResourceID();
}

const ezJoltMaterial* ezJoltQueryShapeActorComponent::GetJoltMaterial() const
{
  if (m_hSurface.IsValid())
  {
    ezResourceLock<ezSurfaceResource> pSurface(m_hSurface, ezResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      return static_cast<ezJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  return nullptr;
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltQueryShapeActorComponent);
