#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxCharacterControllerComponent.h>
#include <PhysXPlugin/PhysXWorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameUtils/Components/InputComponent.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxCharacterControllerComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Capsule Height", m_fCapsuleHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("Capsule Radius", m_fCapsuleRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.1f, 5.0f)),
    EZ_MEMBER_PROPERTY("Max Step Height", m_fMaxStepHeight)->AddAttributes(new ezDefaultValueAttribute(0.3f), new ezClampValueAttribute(0.0f, 5.0f)),
    EZ_MEMBER_PROPERTY("Walk Speed", m_fWalkSpeed)->AddAttributes(new ezDefaultValueAttribute(3.0f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("Rotate Speed", m_RotateSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90.0f)), new ezClampValueAttribute(ezAngle::Degree(1.0f), ezAngle::Degree(360.0f))),
    EZ_MEMBER_PROPERTY("Max Slope Angle", m_MaxClimbingSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(40.0f)), new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(80.0f))),
    EZ_MEMBER_PROPERTY("Force Slope Sliding", m_bForceSlopeSliding)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Constrained Climb Mode", m_bConstrainedClimbingMode),
    EZ_MEMBER_PROPERTY("Collision Layer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(ezInputComponentMessage, InputComponentMessageHandler),
  EZ_END_MESSAGEHANDLERS
EZ_END_COMPONENT_TYPE();

ezPxCharacterControllerComponent::ezPxCharacterControllerComponent()
{
  m_pController = nullptr;
  m_vRelativeMoveDirection.SetZero();

  m_uiCollisionLayer = 0;
  m_fCapsuleHeight = 1.0f;
  m_fCapsuleRadius = 0.25f;
  m_fCapsuleHeight = 0.3f;
  m_MaxClimbingSlope = ezAngle::Degree(40.0f);
  m_bForceSlopeSliding = true;
  m_bConstrainedClimbingMode = false;
  m_fWalkSpeed = 3.0f;
  m_RotateSpeed = ezAngle::Degree(90.0f);
}


void ezPxCharacterControllerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fCapsuleHeight;
  s << m_fCapsuleRadius;
  s << m_fCapsuleHeight;
  s << m_MaxClimbingSlope;
  s << m_bForceSlopeSliding;
  s << m_bConstrainedClimbingMode;
  s << m_fWalkSpeed;
  s << m_RotateSpeed;
  s << m_uiCollisionLayer;
}


void ezPxCharacterControllerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fCapsuleHeight;
  s >> m_fCapsuleRadius;
  s >> m_fCapsuleHeight;
  s >> m_MaxClimbingSlope;
  s >> m_bForceSlopeSliding;
  s >> m_bConstrainedClimbingMode;
  s >> m_fWalkSpeed;
  s >> m_RotateSpeed;
  s >> m_uiCollisionLayer;
}

class ezPxCharacterFilter : public PxQueryFilterCallback
{
public:


  virtual PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
  {
    queryFlags = (PxHitFlags)0;

    // trigger the contact callback for pairs (A,B) where
    // the filter mask of A contains the ID of B and vice versa.
    if ((filterData.word0 & shape->getQueryFilterData().word1) || (shape->getQueryFilterData().word0 & filterData.word1))
    {
      queryFlags |= PxHitFlag::eDEFAULT;
      return PxQueryHitType::eBLOCK;
    }

    return PxQueryHitType::eNONE;
  }

  virtual PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override
  {
    return PxQueryHitType::eNONE;
  }

};

static ezPxCharacterFilter g_CharFilter;

void ezPxCharacterControllerComponent::Update()
{
  if (m_pController == nullptr)
    return;

  const float tDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

  ezPhysXSceneModule* pModule = static_cast<ezPhysXSceneModule*>(GetManager()->GetUserData());

  m_vRelativeMoveDirection = GetOwner()->GetGlobalRotation() * m_vRelativeMoveDirection * m_fWalkSpeed;

  m_vRelativeMoveDirection += pModule->GetCharacterGravity() * tDiff;

  PxVec3 mov;
  mov.x = m_vRelativeMoveDirection.x;
  mov.y = m_vRelativeMoveDirection.y;
  mov.z = m_vRelativeMoveDirection.z;

  /// \todo Filter stuff ?
  PxControllerFilters charFilter;
  PxFilterData filter;
  charFilter.mCCTFilterCallback = nullptr;
  charFilter.mFilterCallback =  &g_CharFilter;
  charFilter.mFilterData = &filter;
  charFilter.mFilterFlags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

  {
    filter.word0 = EZ_BIT(m_uiCollisionLayer);
    filter.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(m_uiCollisionLayer);
    filter.word2 = 0;
    filter.word3 = 0;
  }

  m_pController->move(mov, 0.1f * m_fWalkSpeed * tDiff, tDiff, charFilter);

  m_vRelativeMoveDirection.SetZero();

  auto pos = m_pController->getPosition();

  GetOwner()->SetGlobalPosition(ezVec3((float)pos.x, (float)pos.y, (float)pos.z));

  if (m_RotateZ.GetRadian() != 0.0f)
  {
    ezQuat qRotZ;
    qRotZ.SetFromAxisAndAngle(ezVec3(0, 0, 1), m_RotateZ);

    GetOwner()->SetGlobalRotation(qRotZ * GetOwner()->GetGlobalRotation());

    m_RotateZ.SetRadian(0.0);
  }
}

ezComponent::Initialization ezPxCharacterControllerComponent::Initialize()
{
  ezPhysXSceneModule* pModule = static_cast<ezPhysXSceneModule*>(GetManager()->GetUserData());

  m_vRelativeMoveDirection.SetZero();

  const auto pos = GetOwner()->GetGlobalPosition();
  const auto rot = GetOwner()->GetGlobalRotation();

  PxTransform t = PxTransform::createIdentity();
  t.p = PxVec3(pos.x, pos.y, pos.z);
  t.q = PxQuat(rot.v.x, rot.v.y, rot.v.z, rot.w);

  PxCapsuleControllerDesc cd;
  cd.climbingMode = m_bConstrainedClimbingMode ? PxCapsuleClimbingMode::eCONSTRAINED : PxCapsuleClimbingMode::eEASY;
  cd.height = ezMath::Max(m_fCapsuleHeight, 0.0f);
  cd.radius = ezMath::Max(m_fCapsuleRadius, 0.0f);
  cd.nonWalkableMode = m_bForceSlopeSliding ? PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING : PxControllerNonWalkableMode::ePREVENT_CLIMBING;
  cd.position.set(pos.x, pos.y, pos.z);
  cd.slopeLimit = ezMath::Cos(m_MaxClimbingSlope);
  cd.stepOffset = m_fMaxStepHeight;
  cd.upDirection = PxVec3(0, 0, 1);
  cd.userData = this;
  cd.material = ezPhysX::GetSingleton()->GetDefaultMaterial();

  if (!cd.isValid())
  {
    ezLog::Error("The Character Controller configuration is invalid.");
    return ezComponent::Initialization::Done;
  }

  m_pController = static_cast<PxCapsuleController*>(pModule->GetCharacterManager()->createController(cd));

  EZ_ASSERT_DEV(m_pController != nullptr, "Failed to create character controller");

  return ezComponent::Initialization::Done;
}

void ezPxCharacterControllerComponent::Deinitialize()
{
  if (m_pController)
  {
    /// \todo world module is shut down first -> bad order
    //m_pController->release();
    m_pController = nullptr;
  }
}

void ezPxCharacterControllerComponent::InputComponentMessageHandler(ezInputComponentMessage& msg)
{
  float f = msg.m_fValue;

  if (ezStringUtils::IsEqual(msg.m_szAction, "MoveForwards"))
  {
    m_vRelativeMoveDirection += ezVec3(f, 0, 0);
    return;
  }

  if (ezStringUtils::IsEqual(msg.m_szAction, "MoveBackwards"))
  {
    m_vRelativeMoveDirection += ezVec3(-f, 0, 0);
    return;
  }

  if (ezStringUtils::IsEqual(msg.m_szAction, "StrafeLeft"))
  {
    m_vRelativeMoveDirection += ezVec3(0, -f, 0);
    return;
  }

  if (ezStringUtils::IsEqual(msg.m_szAction, "StrafeRight"))
  {
    m_vRelativeMoveDirection += ezVec3(0, f, 0);
    return;
  }

  if (ezStringUtils::IsEqual(msg.m_szAction, "RotateLeft"))
  {
    m_RotateZ -= m_RotateSpeed * f;
    return;
  }

  if (ezStringUtils::IsEqual(msg.m_szAction, "RotateRight"))
  {
    m_RotateZ += m_RotateSpeed * f;
    return;
  }
}


