#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxCharacterControllerComponent.h>
#include <PhysXPlugin/PhysXWorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameUtils/Components/InputComponent.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxCharacterControllerComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Capsule Height", m_fCapsuleHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("Capsule Radius", m_fCapsuleRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.1f, 5.0f)),
    EZ_MEMBER_PROPERTY("Max Step Height", m_fMaxStepHeight)->AddAttributes(new ezDefaultValueAttribute(0.3f), new ezClampValueAttribute(0.0f, 5.0f)),
    EZ_MEMBER_PROPERTY("Jump Impulse", m_fJumpImpulse)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 5.0f)),
    EZ_MEMBER_PROPERTY("Walk Speed", m_fWalkSpeed)->AddAttributes(new ezDefaultValueAttribute(3.0f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("Run Speed", m_fRunSpeed)->AddAttributes(new ezDefaultValueAttribute(6.0f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("Air Speed", m_fAirSpeed)->AddAttributes(new ezDefaultValueAttribute(2.5f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("Air Friction", m_fAirFriction)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("Rotate Speed", m_RotateSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90.0f)), new ezClampValueAttribute(ezAngle::Degree(1.0f), ezAngle::Degree(360.0f))),
    EZ_MEMBER_PROPERTY("Max Slope Angle", m_MaxClimbingSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(40.0f)), new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(80.0f))),
    EZ_MEMBER_PROPERTY("Force Slope Sliding", m_bForceSlopeSliding)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Constrained Climb Mode", m_bConstrainedClimbingMode),
    EZ_MEMBER_PROPERTY("Collision Layer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezTriggerMessage, TriggerMessageHandler),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCapsuleManipulatorAttribute("Capsule Height", "Capsule Radius"),
    new ezCapsuleVisualizerAttribute("Capsule Height", "Capsule Radius"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

ezPxCharacterControllerComponent::ezPxCharacterControllerComponent()
{
  m_pController = nullptr;
  m_vRelativeMoveDirection.SetZero();

  m_fCapsuleHeight = 1.0f;
  m_fCapsuleRadius = 0.25f;
  m_fMaxStepHeight = 0.3f;
  m_fWalkSpeed = 3.0f;
  m_fRunSpeed = 6.0f;
  m_fAirSpeed = 2.5f;
  m_fAirFriction = 0.5f;
  m_RotateSpeed = ezAngle::Degree(90.0f);
  m_MaxClimbingSlope = ezAngle::Degree(40.0f);
  m_bForceSlopeSliding = true;
  m_bConstrainedClimbingMode = false;
  m_uiCollisionLayer = 0;
  m_fJumpImpulse = 1.0f;
  m_fVelocityUp = 0.0f;
  m_vVelocityLateral.SetZero();
}


void ezPxCharacterControllerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fCapsuleHeight;
  s << m_fCapsuleRadius;
  s << m_fMaxStepHeight;
  s << m_MaxClimbingSlope;
  s << m_bForceSlopeSliding;
  s << m_bConstrainedClimbingMode;
  s << m_fWalkSpeed;
  s << m_fRunSpeed;
  s << m_fAirSpeed;
  s << m_fAirFriction;
  s << m_RotateSpeed;
  s << m_uiCollisionLayer;
  s << m_fJumpImpulse;
}


void ezPxCharacterControllerComponent::DeserializeComponent(ezWorldReader& stream)
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
  s >> m_fWalkSpeed;
  s >> m_fRunSpeed;
  s >> m_fAirSpeed;
  s >> m_fAirFriction;
  s >> m_RotateSpeed;
  s >> m_uiCollisionLayer;
  s >> m_fJumpImpulse;
}

void ezPxCharacterControllerComponent::Update()
{
  if (m_pController == nullptr)
    return;

  const float tDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();
  ezPhysXWorldModule* pModule = static_cast<ezPhysXWorldModule*>(GetManager()->GetUserData());

  

  PxControllerState state;
  m_pController->getState(state);
  const bool isOnGround = (state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN) != 0;
  const bool touchesCeiling = (state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_UP) != 0;
  const bool canJump = isOnGround && !touchesCeiling;
  const bool wantsJump = m_InputStateBits & InputStateBits::Jump;

  const float fGravityFactor = 1.0f;
  const float fJumpFactor = 1.0f;// 0.01f;
  const float fGravity = pModule->GetCharacterGravity().z * fGravityFactor * tDiff;

  if (touchesCeiling || isOnGround)
  {
    m_fVelocityUp = 0.0f;
  }

  // update gravity
  {
    m_fVelocityUp += fGravity; /// \todo allow other gravity directions
  }

  float fIntendedSpeed = m_fAirSpeed;

  if (isOnGround)
  {
    fIntendedSpeed = (m_InputStateBits & InputStateBits::Run) ? m_fRunSpeed : m_fWalkSpeed;
  }

  const ezVec3 vIntendedMovement = GetOwner()->GetGlobalRotation() * m_vRelativeMoveDirection * fIntendedSpeed;

  if (wantsJump && canJump)
  {
    m_fVelocityUp = m_fJumpImpulse / tDiff * fJumpFactor;
  }

  ezVec3 vNewVelocity(0.0f);

  if (isOnGround)
  {
    vNewVelocity = vIntendedMovement;
    vNewVelocity.z = m_fVelocityUp;
  }
  else
  {
    m_vVelocityLateral *= ezMath::Pow(1.0f - m_fAirFriction, tDiff);

    vNewVelocity = m_vVelocityLateral + vIntendedMovement;
    vNewVelocity.z = m_fVelocityUp;
  }

  auto posBefore = m_pController->getPosition();
  //{
    const ezVec3 vMoveDiff = vNewVelocity * tDiff;

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

    m_pController->move(PxVec3(vMoveDiff.x, vMoveDiff.y, vMoveDiff.z), 0.5f * fGravity, tDiff, charFilter);
  //}

  auto posAfter = m_pController->getPosition();

  /// After move forwards, sweep test downwards to stick character to floor and detect falling
  if (isOnGround && (!wantsJump || !canJump))
  {
    ezTransform t;
    t.SetIdentity();
    t.m_vPosition.Set((float)posAfter.x, (float)posAfter.y, (float)posAfter.z);

    float fSweepDistance;
    ezVec3 vSweepPosition, vSweepNormal;

    if (pModule->SweepTestCapsule(t, ezVec3(0, 0, -1), m_fCapsuleRadius, m_fCapsuleHeight, m_fMaxStepHeight, m_uiCollisionLayer, fSweepDistance, vSweepPosition, vSweepNormal))
    {
      m_pController->move(PxVec3(0, 0, -fSweepDistance), 0.5f * fGravity, 0.0f, charFilter);

      //ezLog::Info("Floor Distance: %.2f (%.2f | %.2f | %.2f) -> (%.2f | %.2f | %.2f), Radius: %.2f, Height: %.2f", fSweepDistance, t.m_vPosition.x, t.m_vPosition.y, t.m_vPosition.z, vSweepPosition.x, vSweepPosition.y, vSweepPosition.z, m_fCapsuleRadius, m_fCapsuleHeight);
    }
    else
    {
      //ezLog::Dev("Falling");
    }

    posAfter = m_pController->getPosition();
  }

  {
    auto actualMoveDiff = posAfter - posBefore;

    ezVec3 vVelocity(actualMoveDiff.x, actualMoveDiff.y, actualMoveDiff.z);
    vVelocity /= tDiff;

    // when touching ground store lateral velocity
    if (isOnGround)
      m_vVelocityLateral.Set(vVelocity.x, vVelocity.y, 0.0f);
    else
    {
      // otherwise clamp stored lateral velocity by actual velocity

      ezVec3 vRealDirLateral(vVelocity.x, vVelocity.y, 0);

      if (!vRealDirLateral.IsZero())
      {
        vRealDirLateral.Normalize();

        const float fSpeedAlongRealDir = vRealDirLateral.Dot(m_vVelocityLateral);

        m_vVelocityLateral.SetLength(fSpeedAlongRealDir);
      }
      else
        m_vVelocityLateral.SetZero();
    }
  }


  GetOwner()->SetGlobalPosition(ezVec3((float)posAfter.x, (float)posAfter.y, (float)posAfter.z));

  if (m_RotateZ.GetRadian() != 0.0f)
  {
    ezQuat qRotZ;
    qRotZ.SetFromAxisAndAngle(ezVec3(0, 0, 1), m_RotateZ);

    GetOwner()->SetGlobalRotation(qRotZ * GetOwner()->GetGlobalRotation());

    m_RotateZ.SetRadian(0.0);
  }

  // reset all, expect new input
  m_vRelativeMoveDirection.SetZero();
  m_InputStateBits = 0;
}

ezComponent::Initialization ezPxCharacterControllerComponent::Initialize()
{
  ezPhysXWorldModule* pModule = static_cast<ezPhysXWorldModule*>(GetManager()->GetUserData());

  m_vRelativeMoveDirection.SetZero();
  m_InputStateBits = 0;
  m_fVelocityUp = 0.0f;
  m_vVelocityLateral.SetZero();

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

void ezPxCharacterControllerComponent::TriggerMessageHandler(ezTriggerMessage& msg)
{
  float f = msg.m_fTriggerValue;

  if (msg.m_UsageStringHash == ezTempHashedString("MoveForwards").GetHash())
  {
    m_vRelativeMoveDirection += ezVec3(1, 0, 0);
    return;
  }

  if (msg.m_UsageStringHash == ezTempHashedString("MoveBackwards").GetHash())
  {
    m_vRelativeMoveDirection += ezVec3(-1, 0, 0);
    return;
  }

  if (msg.m_UsageStringHash == ezTempHashedString("StrafeLeft").GetHash())
  {
    m_vRelativeMoveDirection += ezVec3(0, -1, 0);
    return;
  }

  if (msg.m_UsageStringHash == ezTempHashedString("StrafeRight").GetHash())
  {
    m_vRelativeMoveDirection += ezVec3(0, 1, 0);
    return;
  }

  if (msg.m_UsageStringHash == ezTempHashedString("RotateLeft").GetHash())
  {
    m_RotateZ -= m_RotateSpeed * f;
    return;
  }

  if (msg.m_UsageStringHash == ezTempHashedString("RotateRight").GetHash())
  {
    m_RotateZ += m_RotateSpeed * f;
    return;
  }

  if (msg.m_UsageStringHash == ezTempHashedString("Run").GetHash())
  {
    m_InputStateBits |= InputStateBits::Run;
    return;
  }

  if (msg.m_UsageStringHash == ezTempHashedString("Jump").GetHash())
  {
    if (msg.m_TriggerState == ezTriggerState::Activated)
    {
      m_InputStateBits |= InputStateBits::Jump;
    }

    return;
  }

  if (msg.m_UsageStringHash == ezTempHashedString("Crouch").GetHash())
  {
    m_InputStateBits |= InputStateBits::Crouch;
    return;
  }
}


