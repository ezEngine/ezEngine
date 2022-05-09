#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Components/JoltCenterOfMassComponent.h>
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

ezJoltDynamicActorComponentManager::~ezJoltDynamicActorComponentManager() {}

void ezJoltDynamicActorComponentManager::UpdateDynamicActors()
{
  EZ_PROFILE_SCOPE("UpdateDynamicActors");

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();

  for (auto itActor : pModule->GetActiveActors())
  {
    JPH::BodyID bodyId(itActor.Value());

    JPH::BodyLockRead bodyLock(pSystem->GetBodyLockInterface(), bodyId);
    if (!bodyLock.Succeeded())
      continue;

    const JPH::Body& body = bodyLock.GetBody();

    if (!body.IsDynamic())
      continue;

    ezSimdTransform trans = itActor.Key()->GetOwner()->GetGlobalTransformSimd();

    trans.m_Position = ezJoltConversionUtils::ToSimdVec3(body.GetPosition());
    trans.m_Rotation = ezJoltConversionUtils::ToSimdQuat(body.GetRotation());

    itActor.Key()->GetOwner()->SetGlobalTransform(trans);
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
EZ_BEGIN_COMPONENT_TYPE(ezJoltDynamicActorComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
      EZ_ACCESSOR_PROPERTY("Kinematic", GetKinematic, SetKinematic),
      EZ_MEMBER_PROPERTY("Mass", m_fMass)->AddAttributes(new ezSuffixAttribute(" kg"), new ezClampValueAttribute(0.0f, ezVariant())),
      EZ_MEMBER_PROPERTY("Density", m_fDensity)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezSuffixAttribute(" kg/m^3")),
      EZ_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
      EZ_MEMBER_PROPERTY("LinearDamping", m_fLinearDamping)->AddAttributes(new ezDefaultValueAttribute(0.2f)),
      EZ_MEMBER_PROPERTY("AngularDamping", m_fAngularDamping)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
      EZ_MEMBER_PROPERTY("ContinuousCollisionDetection", m_bCCD),
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
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezJoltDynamicActorComponent::ezJoltDynamicActorComponent()
{
  m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
}

ezJoltDynamicActorComponent::~ezJoltDynamicActorComponent() = default;

void ezJoltDynamicActorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_bKinematic;
  s << m_bCCD;
  s << m_fLinearDamping;
  s << m_fAngularDamping;
  s << m_fDensity;
  s << m_fMass;
  s << m_fGravityFactor;
}

void ezJoltDynamicActorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_bKinematic;
  s >> m_bCCD;
  s >> m_fLinearDamping;
  s >> m_fAngularDamping;
  s >> m_fDensity;
  s >> m_fMass;
  s >> m_fGravityFactor;
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

void ezJoltDynamicActorComponent::SetGravityFactor(float factor)
{
  if (m_fGravityFactor == factor)
    return;

  m_fGravityFactor = factor;

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

  if (pMaterial == nullptr)
    pMaterial = ezJoltCore::GetDefaultMaterial();

  JPH::BodyCreationSettings bodyCfg;

  if (CreateShape(&bodyCfg, m_fDensity).Failed())
  {
    ezLog::Error("Jolt dynamic actor component '{}' has no valid shape.", GetOwner()->GetName());
    return;
  }

  ezJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  bodyCfg.mPosition = ezJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = ezJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType = m_bKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic;
  bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Dynamic);
  bodyCfg.mMotionQuality = m_bCCD ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
  bodyCfg.mLinearDamping = m_fLinearDamping;
  bodyCfg.mAngularDamping = m_fAngularDamping;
  bodyCfg.mMassPropertiesOverride.mMass = m_fMass;
  bodyCfg.mOverrideMassProperties = m_fMass > 0.0f ? JPH::EOverrideMassProperties::CalculateInertia : JPH::EOverrideMassProperties::CalculateMassAndInertia;
  bodyCfg.mGravityFactor = m_fGravityFactor;
  bodyCfg.mRestitution = pMaterial->m_fRestitution;
  bodyCfg.mFriction = pMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<ezUInt64>(pUserData);

  ezVec3 vCenterOfMass(0.0f);
  if (FindCenterOfMass(GetOwner(), vCenterOfMass))
  {
    ezSimdTransform CoMTransform = trans;
    CoMTransform.m_Scale.Set(1.0f);
    CoMTransform.Invert();

    vCenterOfMass = ezSimdConversion::ToVec3(CoMTransform.TransformPosition(ezSimdConversion::ToVec3(vCenterOfMass)));

    JPH::OffsetCenterOfMassShapeSettings com;
    com.mOffset = ezJoltConversionUtils::ToVec3(vCenterOfMass);
    com.mInnerShapePtr = bodyCfg.GetShape();

    bodyCfg.SetShape(com.Create().Get());
  }

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody);

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

  SUPER::OnDeactivated();
}

void ezJoltDynamicActorComponent::AddLinearForce(const ezVec3& vForce)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddForce(JPH::BodyID(m_uiJoltBodyID), ezJoltConversionUtils::ToVec3(vForce));
}

void ezJoltDynamicActorComponent::AddLinearImpulse(const ezVec3& vImpulse)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddImpulse(JPH::BodyID(m_uiJoltBodyID), ezJoltConversionUtils::ToVec3(vImpulse));
}

void ezJoltDynamicActorComponent::AddAngularForce(const ezVec3& vForce)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddTorque(JPH::BodyID(m_uiJoltBodyID), ezJoltConversionUtils::ToVec3(vForce));
}

void ezJoltDynamicActorComponent::AddAngularImpulse(const ezVec3& vImpulse)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddAngularImpulse(JPH::BodyID(m_uiJoltBodyID), ezJoltConversionUtils::ToVec3(vImpulse));
}

void ezJoltDynamicActorComponent::AddForceAtPos(ezMsgPhysicsAddForce& msg)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddForce(JPH::BodyID(m_uiJoltBodyID), ezJoltConversionUtils::ToVec3(msg.m_vForce), ezJoltConversionUtils::ToVec3(msg.m_vGlobalPosition));
}

void ezJoltDynamicActorComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg)
{
  if (m_bKinematic || m_uiJoltBodyID == ezInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddImpulse(JPH::BodyID(m_uiJoltBodyID), ezJoltConversionUtils::ToVec3(msg.m_vImpulse), ezJoltConversionUtils::ToVec3(msg.m_vGlobalPosition));
}

bool ezJoltDynamicActorComponent::FindCenterOfMass(ezGameObject* pRoot, ezVec3& out_CoM) const
{
  ezJoltCenterOfMassComponent* pCOM;
  if (pRoot->TryGetComponentOfBaseType<ezJoltCenterOfMassComponent>(pCOM))
  {
    out_CoM = pRoot->GetGlobalPosition();
    return true;
  }
  else
  {
    auto it = pRoot->GetChildren();

    while (it.IsValid())
    {
      if (FindCenterOfMass(it, out_CoM))
        return true;

      ++it;
    }
  }

  return false;
}
