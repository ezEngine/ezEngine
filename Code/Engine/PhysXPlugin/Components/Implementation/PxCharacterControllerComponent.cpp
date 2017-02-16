#include <PCH.h>
#include <PhysXPlugin/Components/PxCharacterControllerComponent.h>
#include <PhysXPlugin/Components/PxCharacterProxyComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

ezPxCharacterControllerComponentManager::ezPxCharacterControllerComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezPxCharacterControllerComponent, ezBlockStorageType::Compact>(pWorld)
{
}

ezPxCharacterControllerComponentManager::~ezPxCharacterControllerComponentManager()
{
}

void ezPxCharacterControllerComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPxCharacterControllerComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PostTransform;

  this->RegisterUpdateFunction(desc);
}

void ezPxCharacterControllerComponentManager::Deinitialize()
{
}

void ezPxCharacterControllerComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ezPxCharacterControllerComponent* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezPxCharacterControllerComponent, 3)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("JumpImpulse", m_fJumpImpulse)->AddAttributes(new ezDefaultValueAttribute(6.0f), new ezClampValueAttribute(0.0f, 50.0f)),
    EZ_MEMBER_PROPERTY("WalkSpeed", m_fWalkSpeed)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("RunSpeed", m_fRunSpeed)->AddAttributes(new ezDefaultValueAttribute(15.0f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("AirSpeed", m_fAirSpeed)->AddAttributes(new ezDefaultValueAttribute(2.5f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("AirFriction", m_fAirFriction)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("RotateSpeed", m_RotateSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90.0f)), new ezClampValueAttribute(ezAngle::Degree(1.0f), ezAngle::Degree(360.0f))),
    EZ_MEMBER_PROPERTY("PushingForce", m_fPushingForce)->AddAttributes(new ezDefaultValueAttribute(500.0f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezTriggerMessage, OnTrigger),
    EZ_MESSAGE_HANDLER(ezCollisionMessage, OnCollision)
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_COMPONENT_TYPE

ezPxCharacterControllerComponent::ezPxCharacterControllerComponent()
{
  m_InputStateBits = 0;

  m_vRelativeMoveDirection.SetZero();

  m_fWalkSpeed = 5.0f;
  m_fRunSpeed = 15.0f;
  m_fAirSpeed = 2.5f;
  m_fAirFriction = 0.5f;
  m_RotateSpeed = ezAngle::Degree(90.0f);
  m_fJumpImpulse = 6.0f;
  m_fPushingForce = 500.0f;
  m_fVelocityUp = 0.0f;
  m_vVelocityLateral.SetZero();
  m_vExternalVelocity.SetZero();
}


void ezPxCharacterControllerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fWalkSpeed;
  s << m_fRunSpeed;
  s << m_fAirSpeed;
  s << m_fAirFriction;
  s << m_RotateSpeed;
  s << m_fJumpImpulse;
  s << m_fPushingForce;
}


void ezPxCharacterControllerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  if (uiVersion < 2)
  {
    float fDummy; bool bDummy; ezAngle dummyAngle;

    s >> fDummy;
    s >> fDummy;
    s >> fDummy;
    s >> dummyAngle;
    s >> bDummy;
    s >> bDummy;
  }

  s >> m_fWalkSpeed;
  s >> m_fRunSpeed;
  s >> m_fAirSpeed;
  s >> m_fAirFriction;
  s >> m_RotateSpeed;

  if (uiVersion < 2)
  {
    ezUInt32 uiDummy;
    s >> uiDummy;
  }

  s >> m_fJumpImpulse;

  if (uiVersion >= 3)
  {
    s >> m_fPushingForce;
  }
}

void ezPxCharacterControllerComponent::Update()
{
  ezPxCharacterProxyComponent* pProxy = nullptr;
  if (!GetWorld()->TryGetComponent(m_hProxy, pProxy))
    return;

  const float tDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();
  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  auto collisionFlags = pProxy->GetCollisionFlags();
  const bool isOnGround = collisionFlags.IsSet(ezPxCharacterCollisionFlags::Below);
  const bool touchesCeiling = collisionFlags.IsSet(ezPxCharacterCollisionFlags::Above);
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
    m_fVelocityUp = m_fJumpImpulse;
  }

  ezVec3 vNewVelocity(0.0f);

  if (m_vExternalVelocity.IsZero())
  {
    if (isOnGround)
    {
      vNewVelocity = vIntendedMovement;
    }
    else
    {
      m_vVelocityLateral *= ezMath::Pow(1.0f - m_fAirFriction, tDiff);

      vNewVelocity = m_vVelocityLateral + vIntendedMovement;
    }
  }
  else
  {
    vNewVelocity = m_vExternalVelocity;
    m_vExternalVelocity.SetZero();
  }

  vNewVelocity.z = m_fVelocityUp;

  auto posBefore = GetOwner()->GetGlobalPosition();
  //{
    const ezVec3 vMoveDiff = vNewVelocity * tDiff;
    pProxy->Move(vMoveDiff);
  //}

  auto posAfter = GetOwner()->GetGlobalPosition();

  /// After move forwards, sweep test downwards to stick character to floor and detect falling
  if (isOnGround && (!wantsJump || !canJump))
  {
    ezTransform t;
    t.SetIdentity();
    t.m_vPosition.Set((float)posAfter.x, (float)posAfter.y, (float)posAfter.z);

    ezPhysicsHitResult hitResult;
    if (pModule->SweepTestCapsule(pProxy->m_fCapsuleRadius, pProxy->m_fCapsuleHeight, t, ezVec3(0, 0, -1), pProxy->m_fMaxStepHeight, pProxy->m_uiCollisionLayer, hitResult, pProxy->GetShapeId()))
    {
      pProxy->Move(ezVec3(0, 0, -hitResult.m_fDistance));

      //ezLog::Info("Floor Distance: {0} ({1} | {2} | {3}) -> ({4} | {5} | {6}), Radius: {7}, Height: {8}", ezArgF(fSweepDistance, 2), ezArgF(t.m_vPosition.x, 2), ezArgF(t.m_vPosition.y, 2), ezArgF(t.m_vPosition.z, 2), ezArgF(vSweepPosition.x, 2), ezArgF(vSweepPosition.y, 2), ezArgF(vSweepPosition.z, 2), ezArgF(m_fCapsuleRadius, 2), ezArgF(m_fCapsuleHeight, 2));
    }
    else
    {
      //ezLog::Dev("Falling");
    }

    posAfter = GetOwner()->GetGlobalPosition();
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

void ezPxCharacterControllerComponent::OnSimulationStarted()
{
  m_vRelativeMoveDirection.SetZero();
  m_InputStateBits = 0;
  m_fVelocityUp = 0.0f;
  m_vVelocityLateral.SetZero();

  ezPxCharacterProxyComponent* pProxy = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType(pProxy))
  {
    m_hProxy = pProxy->GetHandle();
  }
}

void ezPxCharacterControllerComponent::OnTrigger(ezTriggerMessage& msg)
{
  float f = msg.m_TriggerValue.ConvertTo<float>();

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

void ezPxCharacterControllerComponent::OnCollision(ezCollisionMessage& msg)
{
  ezWorld* pWorld = GetWorld();
  ezGameObject* pOwner = GetOwner();

  if (msg.m_hObjectA == pOwner->GetHandle())
  {
    // This object was the source of collision thus we want to push the other body.
    ezPxDynamicActorComponent* pDynamicActorComponent = nullptr;
    if (pWorld->TryGetComponent(msg.m_hComponentB, pDynamicActorComponent))
    {
      const ezVec3 vImpulse = msg.m_vImpulse;
      if (ezMath::Abs(vImpulse.z) < 0.01f)
      {
        ezVec3 vHitPos = msg.m_vPosition;
        ezVec3 vCenterOfMass = pDynamicActorComponent->GetGlobalCenterOfMass();

        // Move the hit pos closer to the center of mass in the up direction. Otherwise we tip over objects pretty easily.
        vHitPos.z = ezMath::Lerp(vCenterOfMass.z, vHitPos.z, 0.1f);

        const ezVec3 vIntendedMovement = pOwner->GetGlobalRotation() * m_vRelativeMoveDirection;
        const ezVec3 vForce = vIntendedMovement * m_fPushingForce;

        pDynamicActorComponent->AddForceAtPos(vForce, vHitPos);
      }
    }
  }
  else if (msg.m_hObjectB == pOwner->GetHandle())
  {
    // Another object was the source of collision so the CC is pushed by the another body.
    ezPxDynamicActorComponent* pDynamicActorComponent = nullptr;
    if (pWorld->TryGetComponent(msg.m_hComponentA, pDynamicActorComponent))
    {
      if (pDynamicActorComponent->GetKinematic())
      {
        ezVec3 vNormal = -msg.m_vNormal; // the normal is from B to A but we're interested in the other direction
        ezVec3 vVelocity = pDynamicActorComponent->GetOwner()->GetVelocity();
        m_vExternalVelocity = vNormal * ezMath::Max(vNormal.Dot(vVelocity), 0.0f); // reduce effect if normal and velocity are in different directions
      }
    }
  }
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxCharacterControllerComponent);

