#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <Physics/Collision/Shape/OffsetCenterOfMassShape.h>

ezJoltDynamicActorComponentManager::ezJoltDynamicActorComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezJoltDynamicActorComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezJoltDynamicActorComponentManager::~ezJoltDynamicActorComponentManager() = default;

void ezJoltDynamicActorComponentManager::UpdateDynamicActors()
{
  EZ_PROFILE_SCOPE("UpdateDynamicActors");

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();

  for (auto itActor : pModule->GetActiveActors())
  {
    ezJoltDynamicActorComponent* pActor = itActor;

    JPH::BodyID bodyId(pActor->GetJoltBodyID());

    JPH::BodyLockRead bodyLock(pSystem->GetBodyLockInterface(), bodyId);
    if (!bodyLock.Succeeded())
      continue;

    const JPH::Body& body = bodyLock.GetBody();

    if (!body.IsDynamic())
      continue;

    ezSimdTransform trans = pActor->GetOwner()->GetGlobalTransformSimd();

    trans.m_Position = ezJoltConversionUtils::ToSimdVec3(body.GetPosition());
    trans.m_Rotation = ezJoltConversionUtils::ToSimdQuat(body.GetRotation());

    pActor->GetOwner()->SetGlobalTransform(trans);
  }
}

void ezJoltDynamicActorComponentManager::UpdateKinematicActors(ezTime deltaTime)
{
  EZ_PROFILE_SCOPE("UpdateKinematicActors");

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  const float tDiff = deltaTime.AsFloatInSeconds();

  for (auto pKinematicActorComponent : m_KinematicActorComponents)
  {
    JPH::BodyID bodyId(pKinematicActorComponent->m_uiJoltBodyID);

    if (bodyId.IsInvalid())
      continue;

    ezGameObject* pObject = pKinematicActorComponent->GetOwner();

    pObject->UpdateGlobalTransform();

    const ezSimdVec4f pos = pObject->GetGlobalPositionSimd();
    const ezSimdQuat rot = pObject->GetGlobalRotationSimd();

    pBodies->MoveKinematic(bodyId, ezJoltConversionUtils::ToVec3(pos), ezJoltConversionUtils::ToQuat(rot).Normalized(), tDiff);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltDynamicActorComponent, 6, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
      EZ_ACCESSOR_PROPERTY("Kinematic", GetKinematic, SetKinematic),
      EZ_MEMBER_PROPERTY("StartAsleep", m_bStartAsleep),
      EZ_MEMBER_PROPERTY("AllowSleeping", m_bAllowSleeping)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("WeightCategory", m_uiWeightCategory)->AddAttributes(new ezDynamicEnumAttribute("PhysicsWeightCategory"), new ezDefaultValueAttribute(250)),
      EZ_ACCESSOR_PROPERTY("WeightScale", GetWeightValue, SetWeightValue_Scale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
      EZ_ACCESSOR_PROPERTY("Mass", GetWeightValue, SetWeightValue_Mass)->AddAttributes(new ezSuffixAttribute(" kg"), new ezClampValueAttribute(0.0f, ezVariant())),
      EZ_ACCESSOR_PROPERTY("Density", GetWeightValue, SetWeightValue_Density)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezSuffixAttribute(" kg/m^3")),
      EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
      EZ_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
      EZ_MEMBER_PROPERTY("LinearDamping", m_fLinearDamping)->AddAttributes(new ezDefaultValueAttribute(0.2f)),
      EZ_MEMBER_PROPERTY("AngularDamping", m_fAngularDamping)->AddAttributes(new ezDefaultValueAttribute(0.2f)),
      EZ_MEMBER_PROPERTY("ContinuousCollisionDetection", m_bCCD),
      EZ_BITFLAGS_MEMBER_PROPERTY("OnContact", ezOnJoltContact, m_OnContact),
      EZ_ACCESSOR_PROPERTY("CustomCenterOfMass", GetUseCustomCoM, SetUseCustomCoM),
      EZ_MEMBER_PROPERTY("CenterOfMass", m_vCenterOfMass),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
      EZ_MESSAGE_HANDLER(ezMsgPhysicsAddForce, AddForceAtPos),
      EZ_MESSAGE_HANDLER(ezMsgPhysicsAddImpulse, AddImpulseAtPos),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(AddLinearForce, In, "vForce"),
    EZ_SCRIPT_FUNCTION_PROPERTY(AddLinearImpulse, In, "vImpulse"),
    EZ_SCRIPT_FUNCTION_PROPERTY(AddAngularForce, In, "vForce"),
    EZ_SCRIPT_FUNCTION_PROPERTY(AddAngularImpulse, In, "vImpulse"),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTransformManipulatorAttribute("CenterOfMass")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezJoltDynamicActorComponent::ezJoltDynamicActorComponent()
{
  m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
}

ezJoltDynamicActorComponent::~ezJoltDynamicActorComponent() = default;

void ezJoltDynamicActorComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_bKinematic;
  s << m_bCCD;
  s << m_fLinearDamping;
  s << m_fAngularDamping;
  s << m_fWeightValue;
  s << m_fWeightValue; // dummy placeholder
  s << m_fGravityFactor;
  s << m_hSurface;
  s << m_OnContact;
  s << GetUseCustomCoM();
  s << m_vCenterOfMass;
  s << m_bStartAsleep;
  s << m_bAllowSleeping;

  // version 5
  s << m_uiWeightCategory;
}

void ezJoltDynamicActorComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  float fInitialMass = 0.0f;
  float fDensity = 1.0f;

  s >> m_bKinematic;
  s >> m_bCCD;
  s >> m_fLinearDamping;
  s >> m_fAngularDamping;
  s >> fDensity;
  s >> fInitialMass;
  s >> m_fGravityFactor;
  s >> m_hSurface;
  s >> m_OnContact;

  if (uiVersion >= 2)
  {
    bool com;
    s >> com;
    SetUseCustomCoM(com);
    s >> m_vCenterOfMass;
  }

  if (uiVersion >= 3)
  {
    s >> m_bStartAsleep;
  }

  if (uiVersion >= 4)
  {
    s >> m_bAllowSleeping;
  }

  if (uiVersion >= 5)
  {
    s >> m_uiWeightCategory;
    m_fWeightValue = fDensity;
  }
  else
  {
    if (fInitialMass > 0.0f)
    {
      m_uiWeightCategory = 1; // Custom Mass
      m_fWeightValue = fInitialMass;
    }
    else
    {
      m_uiWeightCategory = 2; // Custom Density
      m_fWeightValue = fDensity;
    }
  }
}

void ezJoltDynamicActorComponent::SetKinematic(bool b)
{
  if (m_bKinematic == b)
    return;

  m_bKinematic = b;

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (m_bKinematic && !bodyId.IsInvalid())
  {
    // do not insert this, until we actually have an actor pointer
    GetWorld()->GetOrCreateComponentManager<ezJoltDynamicActorComponentManager>()->m_KinematicActorComponents.PushBack(this);
  }
  else
  {
    GetWorld()->GetOrCreateComponentManager<ezJoltDynamicActorComponentManager>()->m_KinematicActorComponents.RemoveAndSwap(this);
  }

  if (bodyId.IsInvalid())
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();

  {
    JPH::BodyLockWrite bodyLock(pSystem->GetBodyLockInterface(), bodyId);

    if (bodyLock.Succeeded())
    {
      JPH::Body& body = bodyLock.GetBody();
      body.SetMotionType(m_bKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic);
    }
  }

  if (!m_bKinematic && pSystem->GetBodyInterface().IsAdded(bodyId))
  {
    pSystem->GetBodyInterface().ActivateBody(bodyId);
  }
}

void ezJoltDynamicActorComponent::SetGravityFactor(float fFactor)
{
  if (m_fGravityFactor == fFactor)
    return;

  m_fGravityFactor = fFactor;

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (bodyId.IsInvalid())
    return;

  auto* pSystem = GetWorld()->GetOrCreateModule<ezJoltWorldModule>()->GetJoltSystem();

  JPH::BodyLockWrite bodyLock(pSystem->GetBodyLockInterface(), bodyId);

  if (bodyLock.Succeeded())
  {
    bodyLock.GetBody().GetMotionProperties()->SetGravityFactor(m_fGravityFactor);

    if (pSystem->GetBodyInterfaceNoLock().IsAdded(bodyId))
    {
      pSystem->GetBodyInterfaceNoLock().ActivateBody(bodyId);
    }
  }
}

void ezJoltDynamicActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  const ezSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();
  auto* pMaterial = GetJoltMaterial();

  JPH::BodyCreationSettings bodyCfg;

  if (CreateShape(&bodyCfg, m_fWeightValue, pMaterial).Failed())
  {
    ezLog::Error("Jolt dynamic actor component '{}' has no valid shape.", GetOwner()->GetName());
    return;
  }

  if (pMaterial == nullptr)
    pMaterial = ezJoltCore::GetDefaultMaterial();

  ezJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  float fInitialMass = 10.0f;
  if (m_uiWeightCategory != 0)
  {
    if (m_uiWeightCategory == 1) // Custom Mass
    {
      fInitialMass = m_fWeightValue;
    }
    else if (m_uiWeightCategory == 2) // Custom Density
    {
      fInitialMass = 0.0f;
    }
    else
    {
      auto& cat = ezJoltCore::GetWeightCategoryConfig().m_Categories;
      ezUInt32 idx = cat.Find(m_uiWeightCategory);
      if (idx != ezInvalidIndex)
      {
        fInitialMass = cat.GetValue(idx).m_fMass;
        fInitialMass = ezMath::Clamp(fInitialMass * m_fWeightValue, 1.0f, 1000.0f);
      }
    }
  }

  bodyCfg.mPosition = ezJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = ezJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType = m_bKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic;
  bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Dynamic);
  bodyCfg.mMotionQuality = m_bCCD ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
  bodyCfg.mAllowSleeping = m_bAllowSleeping;
  bodyCfg.mLinearDamping = m_fLinearDamping;
  bodyCfg.mAngularDamping = m_fAngularDamping;
  bodyCfg.mMassPropertiesOverride.mMass = fInitialMass;
  bodyCfg.mOverrideMassProperties = fInitialMass > 0.0f ? JPH::EOverrideMassProperties::CalculateInertia : JPH::EOverrideMassProperties::CalculateMassAndInertia;
  bodyCfg.mGravityFactor = m_fGravityFactor;
  bodyCfg.mRestitution = pMaterial->m_fRestitution;
  bodyCfg.mFriction = pMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<ezUInt64>(pUserData);

  if (GetUseCustomCoM())
  {
    const ezVec3 vGlobalScale = GetOwner()->GetGlobalScaling();
    const float scale = ezMath::Min(vGlobalScale.x, vGlobalScale.y, vGlobalScale.z);
    auto vLocalCenterOfMass = ezSimdVec4f(scale * m_vCenterOfMass.x, scale * m_vCenterOfMass.y, scale * m_vCenterOfMass.z);
    auto vPrevCoM = ezJoltConversionUtils::ToSimdVec3(bodyCfg.GetShape()->GetCenterOfMass());

    auto vComShift = vLocalCenterOfMass - vPrevCoM;

    JPH::OffsetCenterOfMassShapeSettings com;
    com.mOffset = ezJoltConversionUtils::ToVec3(vComShift);
    com.mInnerShapePtr = bodyCfg.GetShape();

    bodyCfg.SetShape(com.Create().Get());
  }

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  EZ_ASSERT_DEV(pBody != nullptr, "Jolt body creation failed. You need to increase the maximum number of bodies.");

  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, !m_bStartAsleep);

  if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<ezJoltDynamicActorComponentManager>()->m_KinematicActorComponents.PushBack(this);
  }
}

void ezJoltDynamicActorComponent::OnDeactivated()
{
  if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<ezJoltDynamicActorComponentManager>()->m_KinematicActorComponents.RemoveAndSwap(this);
  }

  auto allConstraints = m_Constraints;
  m_Constraints.Clear();
  m_Constraints.Compact();

  ezJoltMsgDisconnectConstraints msg;
  msg.m_pActor = this;
  msg.m_uiJoltBodyID = GetJoltBodyID();

  ezWorld* pWorld = GetWorld();

  for (ezComponentHandle hConstraint : allConstraints)
  {
    pWorld->SendMessage(hConstraint, msg);
  }

  SUPER::OnDeactivated();
}

void ezJoltDynamicActorComponent::AddLinearForce(const ezVec3& vForce)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  const JPH::BodyID bodyId(m_uiJoltBodyID);

  if (pBodies->IsAdded(bodyId))
  {
    pBodies->AddForce(bodyId, ezJoltConversionUtils::ToVec3(vForce));
  }
}

void ezJoltDynamicActorComponent::AddLinearImpulse(const ezVec3& vImpulse)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  const JPH::BodyID bodyId(m_uiJoltBodyID);

  if (pBodies->IsAdded(bodyId))
  {
    pBodies->AddImpulse(bodyId, ezJoltConversionUtils::ToVec3(vImpulse));
  }
}

void ezJoltDynamicActorComponent::AddAngularForce(const ezVec3& vForce)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  const JPH::BodyID bodyId(m_uiJoltBodyID);

  if (pBodies->IsAdded(bodyId))
  {
    pBodies->AddTorque(bodyId, ezJoltConversionUtils::ToVec3(vForce));
  }
}

void ezJoltDynamicActorComponent::AddAngularImpulse(const ezVec3& vImpulse)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  const JPH::BodyID bodyId(m_uiJoltBodyID);

  if (pBodies->IsAdded(bodyId))
  {
    pBodies->AddAngularImpulse(bodyId, ezJoltConversionUtils::ToVec3(vImpulse));
  }
}

void ezJoltDynamicActorComponent::AddConstraint(ezComponentHandle hComponent)
{
  m_Constraints.PushBack(hComponent);
}

void ezJoltDynamicActorComponent::RemoveConstraint(ezComponentHandle hComponent)
{
  m_Constraints.RemoveAndSwap(hComponent);
}

void ezJoltDynamicActorComponent::AddForceAtPos(ezMsgPhysicsAddForce& ref_msg)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  const JPH::BodyID bodyId(m_uiJoltBodyID);

  if (pBodies->IsAdded(bodyId))
  {
    pBodies->AddForce(bodyId, ezJoltConversionUtils::ToVec3(ref_msg.m_vForce), ezJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
  }
}

void ezJoltDynamicActorComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  GetWorld()->GetModule<ezJoltWorldModule>()->AddImpulse(m_uiJoltBodyID, ref_msg.m_vImpulse, ref_msg.m_vGlobalPosition);
}

float ezJoltDynamicActorComponent::GetMass() const
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return 0.0f;

  auto& bodyLockInterface = GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyLockInterface();
  JPH::BodyLockRead bodyLock(bodyLockInterface, JPH::BodyID(m_uiJoltBodyID));

  const float fInverseMass = bodyLock.GetBody().GetMotionProperties()->GetInverseMass();
  return fInverseMass > 0.0f ? 1.0f / fInverseMass : 0.0f;
}

const ezJoltMaterial* ezJoltDynamicActorComponent::GetJoltMaterial() const
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

void ezJoltDynamicActorComponent::SetWeightValue_Scale(float fValue)
{
  if (m_uiWeightCategory >= 10)
    return;

  m_fWeightValue = fValue;
}

void ezJoltDynamicActorComponent::SetWeightValue_Mass(float fValue)
{
  if (m_uiWeightCategory != 1) // Custom Mass
    return;

  m_fWeightValue = fValue;
}

void ezJoltDynamicActorComponent::SetWeightValue_Density(float fValue)
{
  if (m_uiWeightCategory != 2) // Custom Density
    return;

  m_fWeightValue = fValue;
}

void ezJoltDynamicActorComponent::SetSurfaceFile(ezStringView sFile)
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

ezStringView ezJoltDynamicActorComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltDynamicActorComponent);
