#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxCharacterProxyComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

namespace
{
  static ezBitflags<ezPxCharacterCollisionFlags> ConvertCollisionFlags(PxU32 pxFlags)
  {
    ezBitflags<ezPxCharacterCollisionFlags> result = ezPxCharacterCollisionFlags::None;

    if ((pxFlags & PxControllerCollisionFlag::eCOLLISION_SIDES) != 0)
    {
      result.Add(ezPxCharacterCollisionFlags::Sides);
    }

    if ((pxFlags & PxControllerCollisionFlag::eCOLLISION_UP) != 0)
    {
      result.Add(ezPxCharacterCollisionFlags::Above);
    }

    if ((pxFlags & PxControllerCollisionFlag::eCOLLISION_DOWN) != 0)
    {
      result.Add(ezPxCharacterCollisionFlags::Below);
    }

    return result;
  }
}

//////////////////////////////////////////////////////////////////////////

PxControllerBehaviorFlags ezPxControllerBehaviorCallback::getBehaviorFlags(const PxShape& shape, const PxActor& actor)
{
  const PxRigidDynamic* pDynamicRigidBody = actor.isRigidDynamic();
  if (pDynamicRigidBody != nullptr && pDynamicRigidBody->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
  {
    return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT | PxControllerBehaviorFlag::eCCT_SLIDE;
  }

  return PxControllerBehaviorFlags(0);
}

PxControllerBehaviorFlags ezPxControllerBehaviorCallback::getBehaviorFlags(const PxController& controller)
{
  return PxControllerBehaviorFlags(0);
}

PxControllerBehaviorFlags ezPxControllerBehaviorCallback::getBehaviorFlags(const PxObstacle& obstacle)
{
  return PxControllerBehaviorFlags(0);
}

//////////////////////////////////////////////////////////////////////////

void ezPxControllerHitCallback::onShapeHit(const PxControllerShapeHit& hit)
{
  PxRigidDynamic* pDynamicRigidBody = hit.actor->isRigidDynamic();
  if (pDynamicRigidBody != nullptr && !pDynamicRigidBody->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
  {
    ezPxCharacterProxyComponent* pCharacterProxyComponent = ezPxUserData::GetCharacterProxyComponent(hit.controller->getUserData());
    ezPxDynamicActorComponent* pDynamicActorComponent = ezPxUserData::GetDynamicActorComponent(hit.actor->userData);

    ezGameObject* pCharacterObject = pCharacterProxyComponent->GetOwner();
    const float fMass = hit.controller->getActor()->getMass();

    ezCollisionMessage msg;

    msg.m_hObjectA = pCharacterObject->GetHandle();
    msg.m_hObjectB = pDynamicActorComponent->GetOwner()->GetHandle();

    msg.m_hComponentA = pCharacterProxyComponent->GetHandle();
    msg.m_hComponentB = pDynamicActorComponent->GetHandle();

    msg.m_vPosition = ezPxConversionUtils::ToVec3(hit.worldPos);
    msg.m_vNormal = ezPxConversionUtils::ToVec3(hit.worldNormal);
    msg.m_vImpulse = ezPxConversionUtils::ToVec3(hit.dir) * fMass;

    pCharacterObject->SendMessage(msg);
  }
}

void ezPxControllerHitCallback::onControllerHit(const PxControllersHit& hit)
{
  // do nothing for now
}

void ezPxControllerHitCallback::onObstacleHit(const PxControllerObstacleHit& hit)
{
  // do nothing for now
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezPxCharacterProxyComponent, 2)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CapsuleHeight", m_fCapsuleHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("CapsuleRadius", m_fCapsuleRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.1f, 5.0f)),
    EZ_MEMBER_PROPERTY("Mass", m_fMass)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.01f, ezVariant())),
    EZ_MEMBER_PROPERTY("MaxStepHeight", m_fMaxStepHeight)->AddAttributes(new ezDefaultValueAttribute(0.3f), new ezClampValueAttribute(0.0f, 5.0f)),
    EZ_MEMBER_PROPERTY("MaxSlopeAngle", m_MaxClimbingSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(40.0f)), new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(80.0f))),
    EZ_MEMBER_PROPERTY("ForceSlopeSliding", m_bForceSlopeSliding)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ConstrainedClimbMode", m_bConstrainedClimbingMode),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCapsuleManipulatorAttribute("CapsuleHeight", "CapsuleRadius"),
    new ezCapsuleVisualizerAttribute("CapsuleHeight", "CapsuleRadius"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

ezPxCharacterProxyComponent::ezPxCharacterProxyComponent()
  : m_UserData(this)
{
  m_fCapsuleHeight = 1.0f;
  m_fCapsuleRadius = 0.25f;
  m_fMass = 100.0f;
  m_fMaxStepHeight = 0.3f;
  m_MaxClimbingSlope = ezAngle::Degree(40.0f);
  m_bForceSlopeSliding = true;
  m_bConstrainedClimbingMode = false;
  m_uiCollisionLayer = 0;
  m_uiShapeId = ezInvalidIndex;

  m_pController = nullptr;
}

ezPxCharacterProxyComponent::~ezPxCharacterProxyComponent()
{

}

void ezPxCharacterProxyComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fCapsuleHeight;
  s << m_fCapsuleRadius;
  s << m_fMass;
  s << m_fMaxStepHeight;
  s << m_MaxClimbingSlope;
  s << m_bForceSlopeSliding;
  s << m_bConstrainedClimbingMode;
  s << m_uiCollisionLayer;
}


void ezPxCharacterProxyComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fCapsuleHeight;
  s >> m_fCapsuleRadius;

  if (uiVersion >= 2)
  {
    s >> m_fMass;
  }

  s >> m_fMaxStepHeight;
  s >> m_MaxClimbingSlope;
  s >> m_bForceSlopeSliding;
  s >> m_bConstrainedClimbingMode;
  s >> m_uiCollisionLayer;
}

void ezPxCharacterProxyComponent::Initialize()
{
  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezPxCharacterProxyComponent::Deinitialize()
{
  if (m_pController != nullptr)
  {
    EZ_PX_WRITE_LOCK(*(m_pController->getScene()));

    m_pController->release();
    m_pController = nullptr;
  }

  if (m_uiShapeId != ezInvalidIndex)
  {
    if (ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>())
    {
      pModule->DeleteShapeId(m_uiShapeId);
      m_uiShapeId = ezInvalidIndex;
    }
  }
}

void ezPxCharacterProxyComponent::OnSimulationStarted()
{
  if (!IsActive())
    return;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  m_uiShapeId = pModule->CreateShapeId();

  const ezVec3& pos = GetOwner()->GetGlobalPosition();

  ezCoordinateSystem coordSystem;
  GetWorld()->GetCoordinateSystem(pos, coordSystem);

  PxCapsuleControllerDesc cd;
  cd.position = ezPxConversionUtils::ToExVec3(pos);
  cd.upDirection = ezPxConversionUtils::ToVec3(coordSystem.m_vUpDir);
  cd.slopeLimit = ezMath::Cos(m_MaxClimbingSlope);
  cd.contactOffset = ezMath::Max(m_fCapsuleRadius * 0.1f, 0.01f);
  cd.stepOffset = m_fMaxStepHeight;
  cd.reportCallback = &m_HitCallback;
  cd.behaviorCallback = &m_BehaviorCallback;
  cd.nonWalkableMode = m_bForceSlopeSliding ? PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING : PxControllerNonWalkableMode::ePREVENT_CLIMBING;
  cd.material = ezPhysX::GetSingleton()->GetDefaultMaterial();
  cd.userData = &m_UserData;

  cd.radius = ezMath::Max(m_fCapsuleRadius, 0.0f);
  cd.height = ezMath::Max(m_fCapsuleHeight, 0.0f);
  cd.climbingMode = m_bConstrainedClimbingMode ? PxCapsuleClimbingMode::eCONSTRAINED : PxCapsuleClimbingMode::eEASY;

  if (!cd.isValid())
  {
    ezLog::Error("The Character Controller configuration is invalid.");
    return;
  }

  // Setup filter data
  m_FilterData = ezPhysX::CreateFilterData(m_uiCollisionLayer, m_uiShapeId);

  m_ControllerFilter.mCCTFilterCallback = nullptr;
  m_ControllerFilter.mFilterCallback = nullptr;
  m_ControllerFilter.mFilterData = &m_FilterData;
  m_ControllerFilter.mFilterFlags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

  {
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
    m_pController = static_cast<PxCapsuleController*>(pModule->GetCharacterManager()->createController(cd));
    EZ_ASSERT_DEV(m_pController != nullptr, "Failed to create character controller");

    PxRigidDynamic* pActor = m_pController->getActor();
    pActor->setMass(m_fMass);
    pActor->userData = &m_UserData;

    PxShape* pShape = nullptr;
    pActor->getShapes(&pShape, 1);
    pShape->setSimulationFilterData(m_FilterData);
    pShape->setQueryFilterData(m_FilterData);
  }
}

void ezPxCharacterProxyComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const
{
  msg.m_ResultingLocalBounds.ExpandToInclude(ezBoundingSphere(ezVec3(0, 0, -m_fCapsuleHeight * 0.5f), m_fCapsuleRadius));
  msg.m_ResultingLocalBounds.ExpandToInclude(ezBoundingSphere(ezVec3(0, 0,  m_fCapsuleHeight * 0.5f), m_fCapsuleRadius));
}

ezBitflags<ezPxCharacterCollisionFlags> ezPxCharacterProxyComponent::Move(const ezVec3& vMotion)
{
  if (m_pController != nullptr)
  {
    ezGameObject* pOwner = GetOwner();

    ezVec3 vOldPos = pOwner->GetGlobalPosition();
    const float fElapsedTime = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

    ezPxQueryFilter QueryFilter;
    m_ControllerFilter.mFilterCallback = &QueryFilter;

    EZ_PX_WRITE_LOCK(*(m_pController->getScene()));
    PxControllerCollisionFlags collisionFlags = m_pController->move(ezPxConversionUtils::ToVec3(vMotion), 0.0f, fElapsedTime, m_ControllerFilter);

    ezVec3 vNewPos = ezPxConversionUtils::ToVec3(m_pController->getPosition());
    pOwner->SetGlobalPosition(vNewPos);

    m_ControllerFilter.mFilterCallback = nullptr;

    return ConvertCollisionFlags(collisionFlags);
  }

  return ezPxCharacterCollisionFlags::None;
}

ezBitflags<ezPxCharacterCollisionFlags> ezPxCharacterProxyComponent::GetCollisionFlags() const
{
  if (m_pController != nullptr)
  {
    PxControllerState state;
    m_pController->getState(state);

    return ConvertCollisionFlags(state.collisionFlags);
  }

  return ezPxCharacterCollisionFlags::None;
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxCharacterProxyComponent);

