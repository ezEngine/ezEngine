#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxCharacterProxyComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
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

EZ_BEGIN_COMPONENT_TYPE(ezPxCharacterProxyComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CapsuleHeight", m_fCapsuleHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("CapsuleRadius", m_fCapsuleRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.1f, 5.0f)),
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
{
  m_pController = nullptr;

  m_fCapsuleHeight = 1.0f;
  m_fCapsuleRadius = 0.25f;
  m_fMaxStepHeight = 0.3f;
  m_MaxClimbingSlope = ezAngle::Degree(40.0f);
  m_bForceSlopeSliding = true;
  m_bConstrainedClimbingMode = false;
  m_uiCollisionLayer = 0;
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
}

void ezPxCharacterProxyComponent::OnSimulationStarted()
{
  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  const ezVec3& pos = GetOwner()->GetGlobalPosition();

  PxCapsuleControllerDesc cd;
  cd.climbingMode = m_bConstrainedClimbingMode ? PxCapsuleClimbingMode::eCONSTRAINED : PxCapsuleClimbingMode::eEASY;
  cd.height = ezMath::Max(m_fCapsuleHeight, 0.0f);
  cd.radius = ezMath::Max(m_fCapsuleRadius, 0.0f);
  cd.nonWalkableMode = m_bForceSlopeSliding ? PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING : PxControllerNonWalkableMode::ePREVENT_CLIMBING;
  cd.position.set(pos.x, pos.y, pos.z);
  cd.slopeLimit = ezMath::Cos(m_MaxClimbingSlope);
  cd.stepOffset = m_fMaxStepHeight;
  cd.upDirection = PxVec3(0, 0, 1);
  cd.behaviorCallback = &m_behaviorCallback;
  cd.userData = this;
  cd.material = ezPhysX::GetSingleton()->GetDefaultMaterial();

  if (!cd.isValid())
  {
    ezLog::Error("The Character Controller configuration is invalid.");
    return;
  }

  {
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
    m_pController = static_cast<PxCapsuleController*>(pModule->GetCharacterManager()->createController(cd));
    EZ_ASSERT_DEV(m_pController != nullptr, "Failed to create character controller");
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
    const float fElapsedTime = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

    ezPxQueryFilter CharFilter;

    /// \todo Filter dynamic stuff ?
    PxControllerFilters charFilter;
    PxFilterData filter;
    charFilter.mCCTFilterCallback = nullptr;
    charFilter.mFilterCallback = &CharFilter;
    charFilter.mFilterData = &filter;
    charFilter.mFilterFlags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

    {
      filter.word0 = EZ_BIT(m_uiCollisionLayer);
      filter.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(m_uiCollisionLayer);
      filter.word2 = 0;
      filter.word3 = 0;
    }

    EZ_PX_WRITE_LOCK(*(m_pController->getScene()));
    PxControllerCollisionFlags collisionFlags = m_pController->move(PxVec3(vMotion.x, vMotion.y, vMotion.z), 0.0f, fElapsedTime, charFilter);

    auto position = toVec3(m_pController->getPosition());
    GetOwner()->SetGlobalPosition(ezVec3(position.x, position.y, position.z));

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
