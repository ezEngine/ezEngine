#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxCharacterControllerComponent.h>
#include <PhysXPlugin/Components/PxCharacterShapeComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>

//////////////////////////////////////////////////////////////////////////

ezPxCharacterControllerComponentManager::ezPxCharacterControllerComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezPxCharacterControllerComponent, ezBlockStorageType::Compact>(pWorld)
{
}

ezPxCharacterControllerComponentManager::~ezPxCharacterControllerComponentManager() = default;

void ezPxCharacterControllerComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPxCharacterControllerComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PostAsync;

  this->RegisterUpdateFunction(desc);
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

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxCharacterControllerComponent, 6, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RotateSpeed", m_RotateSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90.0f)), new ezClampValueAttribute(ezAngle::Degree(1.0f), ezAngle::Degree(360.0f))),
    EZ_MEMBER_PROPERTY("WalkSpeed", m_fWalkSpeed)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("RunSpeed", m_fRunSpeed)->AddAttributes(new ezDefaultValueAttribute(15.0f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("AirSpeed", m_fAirSpeed)->AddAttributes(new ezDefaultValueAttribute(2.5f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("AirFriction", m_fAirFriction)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("CrouchHeight", m_fCrouchHeight)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("CrouchSpeed", m_fCrouchSpeed)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("JumpImpulse", m_fJumpImpulse)->AddAttributes(new ezDefaultValueAttribute(6.0f), new ezClampValueAttribute(0.0f, 50.0f)),
    EZ_MEMBER_PROPERTY("PushingForce", m_fPushingForce)->AddAttributes(new ezDefaultValueAttribute(500.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("WalkSurfaceInteraction", GetWalkSurfaceInteraction, SetWalkSurfaceInteraction)->AddAttributes(new ezDynamicStringEnumAttribute("SurfaceInteractionTypeEnum"), new ezDefaultValueAttribute(ezStringView("Footstep"))),
    EZ_MEMBER_PROPERTY("WalkInteractionDistance", m_fWalkInteractionDistance)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("RunInteractionDistance", m_fRunInteractionDistance)->AddAttributes(new ezDefaultValueAttribute(3.0f)),
    EZ_ACCESSOR_PROPERTY("FallbackWalkSurface", GetFallbackWalkSurfaceFile, SetFallbackWalkSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface")),
    EZ_ACCESSOR_PROPERTY("HeadObject", DummyGetter, SetHeadObjectReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgCollision, OnCollision),
    EZ_MESSAGE_HANDLER(ezMsgApplyRootMotion, OnApplyRootMotion)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/PhysX/Character"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPxCharacterControllerComponent::ezPxCharacterControllerComponent()
{
  m_sWalkSurfaceInteraction.Assign("Footstep");
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

  // Version 4
  s << m_sWalkSurfaceInteraction;
  s << m_fWalkInteractionDistance;
  s << m_fRunInteractionDistance;
  s << m_hFallbackWalkSurface;

  // Version 5
  s << m_fCrouchSpeed;

  // Version 6
  s << m_fCrouchHeight;
  stream.WriteGameObjectHandle(m_hHeadObject);
}

void ezPxCharacterControllerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  if (uiVersion < 2)
  {
    float fDummy;
    bool bDummy;
    ezAngle dummyAngle;

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

  if (uiVersion >= 4)
  {
    s >> m_sWalkSurfaceInteraction;
    s >> m_fWalkInteractionDistance;
    s >> m_fRunInteractionDistance;
    s >> m_hFallbackWalkSurface;
  }

  if (uiVersion >= 5)
  {
    s >> m_fCrouchSpeed;
  }

  if (uiVersion >= 6)
  {
    s >> m_fCrouchHeight;
    m_hHeadObject = stream.ReadGameObjectHandle();
  }
}

void ezPxCharacterControllerComponent::Update()
{
  ezPxCharacterShapeComponent* pShape = nullptr;
  if (!GetWorld()->TryGetComponent(m_hCharacterShape, pShape))
    return;

  if (m_bWantsTeleport)
  {
    m_bWantsTeleport = false;
    pShape->TeleportShape(m_vTeleportTo);
    return;
  }

  const float tDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();
  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  auto collisionFlags = pShape->GetCollisionFlags();
  const bool isOnGround = collisionFlags.IsSet(ezPxCharacterShapeCollisionFlags::Below);
  const bool touchesCeiling = collisionFlags.IsSet(ezPxCharacterShapeCollisionFlags::Above);
  const bool canJump = isOnGround && !touchesCeiling;
  const bool wantsJump = (m_uiInputStateBits & InputStateBits::Jump) != 0;
  m_bWantsCrouch = (m_uiInputStateBits & InputStateBits::Crouch) != 0;

  m_bIsTouchingGround = isOnGround;

  const float fGravityFactor = 1.0f;
  const float fJumpFactor = 1.0f; // 0.01f;
  const float fGravity = pModule->GetCharacterGravity().z * fGravityFactor * tDiff;

  if (touchesCeiling)
  {
    m_fVelocityUp = ezMath::Min(m_fVelocityUp, 0.0f);
  }

  if (isOnGround)
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
    fIntendedSpeed = (m_uiInputStateBits & InputStateBits::Run) ? m_fRunSpeed : m_fWalkSpeed;

    if (m_bWantsCrouch)
      fIntendedSpeed = m_fCrouchSpeed;
  }

  const ezVec3 vIntendedMovement = GetOwner()->GetGlobalRotation() * m_vRelativeMoveDirection * fIntendedSpeed;

  const auto posBefore = GetOwner()->GetGlobalPosition();

  if (wantsJump && canJump)
  {
    m_fVelocityUp = m_fJumpImpulse;
  }
  // somehow this messes up the standing/falling state
  // and the CC seems to oscillate between crouching and standing (but you only see that in the physx visual debugger)
  // else if (isOnGround && !pShape->IsCrouching())
  //{
  //  // auto-crouch functionality

  //  ezVec3 vWalkDir = vIntendedMovement;
  //  vWalkDir.NormalizeIfNotZero(ezVec3::ZeroVector());

  //  ezTransform tDestination;
  //  tDestination.m_Rotation.SetIdentity();
  //  tDestination.m_vPosition = posBefore + vWalkDir * pShape->m_fCapsuleRadius;

  //  // if the destination is blocked (standing upright)
  //  if (pModule->OverlapTestCapsule(pShape->m_fCapsuleRadius, pShape->m_fCapsuleHeight, tDestination, pShape->m_uiCollisionLayer,
  //  pShape->GetShapeId()))
  //  {
  //    // but it is not blocked when crouched
  //    if (!pModule->OverlapTestCapsule(pShape->m_fCapsuleRadius, pShape->m_fCapsuleCrouchHeight, tDestination, pShape->m_uiCollisionLayer,
  //    pShape->GetShapeId()))
  //    {
  //      m_bWantsCrouch = true;
  //    }
  //  }
  //}

  ezVec3 vNewVelocity(0.0f);

  float fAddWalkDistance = 0.0f;

  if (isOnGround)
  {
    vNewVelocity = vIntendedMovement;
    fAddWalkDistance = vIntendedMovement.GetLength() + m_vAbsoluteRootMotion.GetLength();
  }
  else
  {
    m_vVelocityLateral *= ezMath::Pow(1.0f - m_fAirFriction, tDiff);

    vNewVelocity = m_vVelocityLateral + vIntendedMovement;
  }

  vNewVelocity.z = m_fVelocityUp;


  {
    const ezVec3 vMoveDiff = vNewVelocity * tDiff;
    RawMove(GetOwner()->GetGlobalRotation() * m_vAbsoluteRootMotion + vMoveDiff);
  }
  auto posAfter = GetOwner()->GetGlobalPosition();

  /// After move forwards, sweep test downwards to stick character to floor and detect falling
  if (isOnGround && (!wantsJump || !canJump))
  {
    m_fAccumulatedWalkDistance += ezMath::Min(fAddWalkDistance, (posAfter - posBefore).GetLength());

    ezPhysicsCastResult castResult;
    if (pShape->TestShapeSweep(castResult, ezVec3(0, 0, -1), pShape->m_fMaxStepHeight))
    {
      RawMove(ezVec3(0, 0, -castResult.m_fDistance));

      // Footstep Surface Interaction
      if (!m_sWalkSurfaceInteraction.IsEmpty())
      {
        ezSurfaceResourceHandle hSurface;
        if (castResult.m_hSurface.IsValid())
          hSurface = castResult.m_hSurface;
        else
          hSurface = m_hFallbackWalkSurface;

        if (hSurface.IsValid())
        {
          const bool bRun = (m_uiInputStateBits & InputStateBits::Run) != 0;
          if (!bRun && m_fAccumulatedWalkDistance >= m_fWalkInteractionDistance || bRun && m_fAccumulatedWalkDistance >= m_fRunInteractionDistance)
          {
            m_fAccumulatedWalkDistance = 0.0f;

            ezResourceLock<ezSurfaceResource> pSurface(hSurface, ezResourceAcquireMode::AllowLoadingFallback);
            pSurface->InteractWithSurface(GetWorld(), ezGameObjectHandle(), castResult.m_vPosition, castResult.m_vNormal, ezVec3(0, 0, 1), m_sWalkSurfaceInteraction, &GetOwner()->GetTeamID());
          }
        }
      }
    }
    else
    {
      m_fAccumulatedWalkDistance = 0.0f;
      // ezLog::Dev("Falling");
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

        m_vVelocityLateral.SetLength(fSpeedAlongRealDir).IgnoreResult();
      }
      else
        m_vVelocityLateral.SetZero();
    }
  }

  if (m_bIsCrouching != m_bWantsCrouch)
  {
    if (m_bWantsCrouch)
    {
      m_fStandingHeight = pShape->GetCurrentHeightValue();

      if (pShape->TryResize(m_fCrouchHeight))
      {
        m_bIsCrouching = true;
        m_fHeadTargetHeight = m_fHeadHeightOffset - (m_fStandingHeight - m_fCrouchHeight);
      }
    }
    else
    {
      if (pShape->TryResize(m_fStandingHeight))
      {
        m_bIsCrouching = false;
        m_fHeadTargetHeight = m_fHeadHeightOffset;
      }
    }
  }

  if (m_RotateZ.GetRadian() != 0.0f)
  {
    ezQuat qRotZ;
    qRotZ.SetFromAxisAndAngle(ezVec3(0, 0, 1), m_RotateZ);

    GetOwner()->SetGlobalRotation(qRotZ * GetOwner()->GetGlobalRotation());

    m_RotateZ.SetRadian(0.0);
  }

  // reset all, expect new input
  m_vRelativeMoveDirection.SetZero();
  m_vAbsoluteRootMotion.SetZero();
  m_uiInputStateBits = 0;

  ezGameObject* pHeadObject;
  if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  {
    ezVec3 pos = pHeadObject->GetLocalPosition();

    float fTimeDiff = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();
    fTimeDiff = ezMath::Max(fTimeDiff, 0.005f); // prevent stuff from breaking at high frame rates
    float fFactor = 1.0f - ezMath::Pow(0.001f, fTimeDiff);
    pos.z = ezMath::Lerp(pos.z, m_fHeadTargetHeight, fFactor);

    pHeadObject->SetLocalPosition(pos);
  }
}

void ezPxCharacterControllerComponent::OnSimulationStarted()
{
  m_vRelativeMoveDirection.SetZero();
  m_vAbsoluteRootMotion.SetZero();
  m_uiInputStateBits = 0;
  m_fVelocityUp = 0.0f;
  m_vVelocityLateral.SetZero();

  ezPxCharacterShapeComponent* pShape = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType(pShape))
  {
    m_hCharacterShape = pShape->GetHandle();
  }
  else
  {
    ezLog::Error("Character controller could not find character shape component (on same parent).");
  }

  ezGameObject* pHeadObject;
  if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  {
    m_fHeadHeightOffset = pHeadObject->GetLocalPosition().z;
    m_fHeadTargetHeight = m_fHeadHeightOffset;
  }
}

void ezPxCharacterControllerComponent::SetFallbackWalkSurfaceFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hFallbackWalkSurface = ezResourceManager::LoadResource<ezSurfaceResource>(szFile);
  }

  if (m_hFallbackWalkSurface.IsValid())
    ezResourceManager::PreloadResource(m_hFallbackWalkSurface);
}

const char* ezPxCharacterControllerComponent::GetFallbackWalkSurfaceFile() const
{
  if (!m_hFallbackWalkSurface.IsValid())
    return "";

  return m_hFallbackWalkSurface.GetResourceID();
}

void ezPxCharacterControllerComponent::SetHeadObjectReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hHeadObject = resolver(szReference, GetHandle(), "HeadObject");
}

void ezPxCharacterControllerComponent::MoveCharacter(ezMsgMoveCharacterController& msg)
{
  const float fDistanceToMove = ezMath::Max(ezMath::Abs((float)(msg.m_fMoveForwards - msg.m_fMoveBackwards)), ezMath::Abs((float)(msg.m_fStrafeRight - msg.m_fStrafeLeft)));

  m_vRelativeMoveDirection += ezVec3((float)(msg.m_fMoveForwards - msg.m_fMoveBackwards), (float)(msg.m_fStrafeRight - msg.m_fStrafeLeft), 0);
  m_vRelativeMoveDirection.NormalizeIfNotZero(ezVec3::ZeroVector()).IgnoreResult();
  m_vRelativeMoveDirection *= fDistanceToMove;

  m_RotateZ += m_RotateSpeed * (float)(msg.m_fRotateRight - msg.m_fRotateLeft);

  if (msg.m_bRun)
  {
    m_uiInputStateBits |= InputStateBits::Run;
  }

  if (msg.m_bJump)
  {
    m_uiInputStateBits |= InputStateBits::Jump;
  }

  if (msg.m_bCrouch)
  {
    m_uiInputStateBits |= InputStateBits::Crouch;
  }
}

void ezPxCharacterControllerComponent::TeleportCharacter(const ezVec3& vGlobalFootPos)
{
  ezPxCharacterShapeComponent* pShape = nullptr;
  if (!GetWorld()->TryGetComponent(m_hCharacterShape, pShape))
    return;

  m_vTeleportTo = vGlobalFootPos;
  m_bWantsTeleport = true;
}

bool ezPxCharacterControllerComponent::IsDestinationUnobstructed(const ezVec3& vGlobalFootPos, float fCharacterHeight)
{
  ezPxCharacterShapeComponent* pShape = nullptr;
  if (!GetWorld()->TryGetComponent(m_hCharacterShape, pShape))
    return false;

  if (fCharacterHeight < 0.01f)
    fCharacterHeight = pShape->GetCurrentHeightValue();

  return !pShape->TestShapeOverlap(vGlobalFootPos, fCharacterHeight);
}

bool ezPxCharacterControllerComponent::IsTouchingGround()
{
  return m_bIsTouchingGround;
}

bool ezPxCharacterControllerComponent::IsCrouching()
{
  return m_bIsCrouching;
}

void ezPxCharacterControllerComponent::OnApplyRootMotion(ezMsgApplyRootMotion& msg)
{
  m_vAbsoluteRootMotion = msg.m_vTranslation;
  m_RotateZ += msg.m_RotationZ;
}

void ezPxCharacterControllerComponent::RawMove(const ezVec3& vMoveDeltaGlobal)
{
  ezPxCharacterShapeComponent* pShape = nullptr;
  if (!GetWorld()->TryGetComponent(m_hCharacterShape, pShape))
    return;

  pShape->MoveShape(vMoveDeltaGlobal);
}

void ezPxCharacterControllerComponent::OnCollision(ezMsgCollision& msg)
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
        ezVec3 vAbs = m_vAbsoluteRootMotion;

        if (!vAbs.IsZero())
        {
          vAbs /= pWorld->GetClock().GetTimeDiff().AsFloatInSeconds();
        }

        ezVec3 vHitPos = msg.m_vPosition;
        ezVec3 vCenterOfMass = pDynamicActorComponent->GetGlobalCenterOfMass();

        // Move the hit pos closer to the center of mass in the up direction. Otherwise we tip over objects pretty easily.
        vHitPos.z = ezMath::Lerp(vCenterOfMass.z, vHitPos.z, 0.1f);

        const ezVec3 vIntendedMovement = pOwner->GetGlobalRotation() * (m_vRelativeMoveDirection + vAbs);
        const ezVec3 vForce = vIntendedMovement * m_fPushingForce;

        ezMsgPhysicsAddForce forceMsg;
        forceMsg.m_vForce = vForce;
        forceMsg.m_vGlobalPosition = vHitPos;
        pDynamicActorComponent->AddForceAtPos(forceMsg);
      }
    }
  }
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxCharacterControllerComponent);
