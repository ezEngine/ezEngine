#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/Stats.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <JoltPlugin/Character/JoltDefaultCharacterComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezCVarBool cvar_JoltCcFootCheck("Jolt.CC.FootCheck", true, ezCVarFlags::Default, "Stay down");

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltDefaultCharacterComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ShapeRadius", m_fShapeRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f)),
    EZ_MEMBER_PROPERTY("CrouchHeight", m_fCylinderHeightCrouch)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("StandHeight", m_fCylinderHeightStand)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("FootRadius", m_fFootRadius)->AddAttributes(new ezDefaultValueAttribute(0.15f)),
    EZ_MEMBER_PROPERTY("WalkSpeedCrouching", m_fWalkSpeedCrouching)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("WalkSpeedStanding", m_fWalkSpeedStanding)->AddAttributes(new ezDefaultValueAttribute(2.5f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("WalkSpeedRunning", m_fWalkSpeedRunning)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("AirSpeed", m_fAirSpeed)->AddAttributes(new ezDefaultValueAttribute(2.5f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("AirFriction", m_fAirFriction)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("MaxStepUp", m_fMaxStepUp)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("MaxStepDown", m_fMaxStepDown)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("JumpImpulse", m_fJumpImpulse)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, 1000.0f)),
    EZ_MEMBER_PROPERTY("RotateSpeed", m_RotateSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(90.0f)), new ezClampValueAttribute(ezAngle::MakeFromDegree(1.0f), ezAngle::MakeFromDegree(360.0f))),
    EZ_ACCESSOR_PROPERTY("WalkSurfaceInteraction", GetWalkSurfaceInteraction, SetWalkSurfaceInteraction)->AddAttributes(new ezDynamicStringEnumAttribute("SurfaceInteractionTypeEnum"), new ezDefaultValueAttribute(ezStringView("Footstep"))),
    EZ_MEMBER_PROPERTY("WalkInteractionDistance", m_fWalkInteractionDistance)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("RunInteractionDistance", m_fRunInteractionDistance)->AddAttributes(new ezDefaultValueAttribute(3.0f)),
    EZ_ACCESSOR_PROPERTY("FallbackWalkSurface", GetFallbackWalkSurfaceFile, SetFallbackWalkSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_ACCESSOR_PROPERTY("HeadObject", DummyGetter, SetHeadObjectReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCapsuleVisualizerAttribute("StandHeight", "ShapeRadius", ezColor::WhiteSmoke, nullptr, ezVisualizerAnchor::NegZ),
    new ezCapsuleVisualizerAttribute("CrouchHeight", "ShapeRadius", ezColor::LightSlateGrey, nullptr, ezVisualizerAnchor::NegZ),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgMoveCharacterController, SetInputState),
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgApplyRootMotion, OnApplyRootMotion),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    //EZ_SCRIPT_FUNCTION_PROPERTY(IsDestinationUnobstructed, In, "globalFootPosition", In, "characterHeight"),
    EZ_SCRIPT_FUNCTION_PROPERTY(TeleportCharacter, In, "globalFootPosition"),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsStandingOnGround),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsSlidingOnGround),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsInAir),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsCrouching),
    EZ_SCRIPT_FUNCTION_PROPERTY(Jump),
    EZ_SCRIPT_FUNCTION_PROPERTY(Run),
    EZ_SCRIPT_FUNCTION_PROPERTY(Crouch),
    EZ_SCRIPT_FUNCTION_PROPERTY(RotateZ, In, "Amount"),
    EZ_SCRIPT_FUNCTION_PROPERTY(Move, In, "Forward", In, "Right"),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezJoltDefaultCharacterComponent::ezJoltDefaultCharacterComponent() = default;
ezJoltDefaultCharacterComponent::~ezJoltDefaultCharacterComponent() = default;

void ezJoltDefaultCharacterComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingSphere::MakeFromCenterAndRadius(ezVec3(0, 0, GetShapeRadius()), GetShapeRadius()), ezInvalidSpatialDataCategory);
  msg.AddBounds(ezBoundingSphere::MakeFromCenterAndRadius(ezVec3(0, 0, GetCurrentCapsuleHeight() - GetShapeRadius()), GetShapeRadius()), ezInvalidSpatialDataCategory);
}

void ezJoltDefaultCharacterComponent::OnApplyRootMotion(ezMsgApplyRootMotion& msg)
{
  m_vAbsoluteRootMotion += msg.m_vTranslation;
  m_InputRotateZ += msg.m_RotationZ;
}

void ezJoltDefaultCharacterComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_RotateSpeed;
  s << m_fShapeRadius;
  s << m_fCylinderHeightCrouch;
  s << m_fCylinderHeightStand;
  s << m_fWalkSpeedCrouching;
  s << m_fWalkSpeedStanding;
  s << m_fWalkSpeedRunning;
  s << m_fMaxStepUp;
  s << m_fMaxStepDown;
  s << m_fJumpImpulse;
  s << m_sWalkSurfaceInteraction;
  s << m_fWalkInteractionDistance;
  s << m_fRunInteractionDistance;
  s << m_hFallbackWalkSurface;
  s << m_fAirFriction;
  s << m_fAirSpeed;
  s << m_fFootRadius;

  inout_stream.WriteGameObjectHandle(m_hHeadObject);
}

void ezJoltDefaultCharacterComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_RotateSpeed;
  s >> m_fShapeRadius;
  s >> m_fCylinderHeightCrouch;
  s >> m_fCylinderHeightStand;
  s >> m_fWalkSpeedCrouching;
  s >> m_fWalkSpeedStanding;
  s >> m_fWalkSpeedRunning;
  s >> m_fMaxStepUp;
  s >> m_fMaxStepDown;
  s >> m_fJumpImpulse;
  s >> m_sWalkSurfaceInteraction;
  s >> m_fWalkInteractionDistance;
  s >> m_fRunInteractionDistance;
  s >> m_hFallbackWalkSurface;
  s >> m_fAirFriction;
  s >> m_fAirSpeed;
  s >> m_fFootRadius;

  m_hHeadObject = inout_stream.ReadGameObjectHandle();

  ResetInternalState();
}

void ezJoltDefaultCharacterComponent::ResetInternalState()
{
  m_fShapeRadius = ezMath::Clamp(m_fShapeRadius, 0.05f, 5.0f);
  m_fFootRadius = ezMath::Clamp(m_fFootRadius, 0.01f, m_fShapeRadius);
  m_fCylinderHeightCrouch = ezMath::Max(m_fCylinderHeightCrouch, 0.01f);
  m_fCylinderHeightStand = ezMath::Max(m_fCylinderHeightStand, m_fCylinderHeightCrouch);
  m_fMaxStepUp = ezMath::Clamp(m_fMaxStepUp, 0.0f, m_fCylinderHeightStand);
  m_fMaxStepDown = ezMath::Clamp(m_fMaxStepDown, 0.0f, m_fCylinderHeightStand);

  m_fNextCylinderHeight = m_fCylinderHeightStand;
  m_fCurrentCylinderHeight = m_fNextCylinderHeight;

  m_vVelocityLateral.SetZero();
  m_fVelocityUp = 0;

  m_fAccumulatedWalkDistance = 0;
}

void ezJoltDefaultCharacterComponent::ResetInputState()
{
  m_vInputDirection.SetZero();
  m_InputRotateZ = ezAngle();
  m_uiInputCrouchBit = 0;
  m_uiInputRunBit = 0;
  m_uiInputJumpBit = 0;
  m_vAbsoluteRootMotion.SetZero();
}

void ezJoltDefaultCharacterComponent::SetInputState(ezMsgMoveCharacterController& ref_msg)
{
  Move(static_cast<float>(ref_msg.m_fMoveForwards - ref_msg.m_fMoveBackwards), static_cast<float>(ref_msg.m_fStrafeRight - ref_msg.m_fStrafeLeft));

  RotateZ((float)(ref_msg.m_fRotateRight - ref_msg.m_fRotateLeft));

  if (ref_msg.m_bRun)
  {
    Run();
  }

  if (ref_msg.m_bJump)
  {
    Jump();
  }

  if (ref_msg.m_bCrouch)
  {
    Crouch();
  }
}

float ezJoltDefaultCharacterComponent::GetCurrentCylinderHeight() const
{
  return m_fCurrentCylinderHeight;
}

float ezJoltDefaultCharacterComponent::GetCurrentCapsuleHeight() const
{
  return GetCurrentCylinderHeight() + 2.0f * GetShapeRadius();
}

float ezJoltDefaultCharacterComponent::GetShapeRadius() const
{
  return m_fShapeRadius;
}

void ezJoltDefaultCharacterComponent::Jump()
{
  m_uiInputJumpBit = 1;
}

void ezJoltDefaultCharacterComponent::Run()
{
  m_uiInputRunBit = 1;
}

void ezJoltDefaultCharacterComponent::Crouch()
{
  m_uiInputCrouchBit = 1;
}

void ezJoltDefaultCharacterComponent::Move(float fForward, float fRight)
{
  const float fDistanceToMove = ezMath::Max(ezMath::Abs(fForward), ezMath::Abs(fRight));

  m_vInputDirection += ezVec2(fForward, fRight);
  m_vInputDirection.NormalizeIfNotZero(ezVec2::MakeZero()).IgnoreResult();
  m_vInputDirection *= fDistanceToMove;
}

void ezJoltDefaultCharacterComponent::RotateZ(float fAmount)
{
  m_InputRotateZ += m_RotateSpeed * fAmount;
}

void ezJoltDefaultCharacterComponent::TeleportCharacter(const ezVec3& vGlobalFootPosition)
{
  TeleportToPosition(vGlobalFootPosition);
}

void ezJoltDefaultCharacterComponent::OnActivated()
{
  SUPER::OnActivated();

  ResetInternalState();

  GetOwner()->UpdateLocalBounds();
}

void ezJoltDefaultCharacterComponent::OnDeactivated()
{
  SUPER::OnDeactivated();
}

JPH::Ref<JPH::Shape> ezJoltDefaultCharacterComponent::MakeNextCharacterShape()
{
  const float fTotalCapsuleHeight = m_fNextCylinderHeight + 2.0f * GetShapeRadius();

  JPH::CapsuleShapeSettings opt;
  opt.mRadius = GetShapeRadius();
  opt.mHalfHeightOfCylinder = 0.5f * m_fNextCylinderHeight;

  JPH::RotatedTranslatedShapeSettings up;
  up.mInnerShapePtr = opt.Create().Get();
  up.mPosition = JPH::Vec3(0, 0, fTotalCapsuleHeight * 0.5f);
  up.mRotation = JPH::Quat::sFromTo(JPH::Vec3::sAxisY(), JPH::Vec3::sAxisZ());

  return up.Create().Get();
}

void ezJoltDefaultCharacterComponent::SetHeadObjectReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hHeadObject = resolver(szReference, GetHandle(), "HeadObject");
}

void ezJoltDefaultCharacterComponent::OnSimulationStarted()
{
  ResetInternalState();

  // creates the CC, so the next shape size must be set already
  SUPER::OnSimulationStarted();

  ezGameObject* pHeadObject;
  if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  {
    m_fHeadHeightOffset = pHeadObject->GetLocalPosition().z;
    m_fHeadTargetHeight = m_fHeadHeightOffset;
  }
}

void ezJoltDefaultCharacterComponent::ApplyRotationZ()
{
  if (m_InputRotateZ.GetRadian() == 0.0f)
    return;

  ezQuat qRotZ = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), m_InputRotateZ);
  m_InputRotateZ.SetRadian(0.0);

  GetOwner()->SetGlobalRotation(qRotZ * GetOwner()->GetGlobalRotation());
}

void ezJoltDefaultCharacterComponent::SetFallbackWalkSurfaceFile(ezStringView sFile)
{
  if (!sFile.IsEmpty())
  {
    m_hFallbackWalkSurface = ezResourceManager::LoadResource<ezSurfaceResource>(sFile);
  }
  else
  {
    m_hFallbackWalkSurface = {};
  }

  if (m_hFallbackWalkSurface.IsValid())
    ezResourceManager::PreloadResource(m_hFallbackWalkSurface);
}

ezStringView ezJoltDefaultCharacterComponent::GetFallbackWalkSurfaceFile() const
{
  return m_hFallbackWalkSurface.GetResourceID();
}

void ezJoltDefaultCharacterComponent::ApplyCrouchState()
{
  if (m_uiInputCrouchBit == m_uiIsCrouchingBit)
    return;

  m_fNextCylinderHeight = m_uiInputCrouchBit ? m_fCylinderHeightCrouch : m_fCylinderHeightStand;

  if (TryChangeShape(MakeNextCharacterShape().GetPtr()).Succeeded())
  {
    m_uiIsCrouchingBit = m_uiInputCrouchBit;
    m_fCurrentCylinderHeight = m_fNextCylinderHeight;
  }
}

void ezJoltDefaultCharacterComponent::StoreLateralVelocity()
{
  const ezVec3 endPosition = GetOwner()->GetGlobalPosition();
  const ezVec3 vVelocity = (endPosition - m_PreviousTransform.m_vPosition) * GetInverseUpdateTimeDelta();

  m_vVelocityLateral.Set(vVelocity.x, vVelocity.y);
}

void ezJoltDefaultCharacterComponent::ClampLateralVelocity()
{
  const ezVec3 endPosition = GetOwner()->GetGlobalPosition();
  const ezVec3 vVelocity = (endPosition - m_PreviousTransform.m_vPosition) * GetInverseUpdateTimeDelta();

  ezVec2 vRealDirLateral(vVelocity.x, vVelocity.y);

  if (!vRealDirLateral.IsZero())
  {
    vRealDirLateral.Normalize();

    const float fSpeedAlongRealDir = vRealDirLateral.Dot(m_vVelocityLateral);

    m_vVelocityLateral = vRealDirLateral * fSpeedAlongRealDir;
  }
  else
    m_vVelocityLateral.SetZero();
}

void ezJoltDefaultCharacterComponent::ClampUpVelocity()
{
  const ezVec3 endPosition = GetOwner()->GetGlobalPosition();
  const ezVec3 vVelocity = (endPosition - m_PreviousTransform.m_vPosition) * GetInverseUpdateTimeDelta();

  m_fVelocityUp = ezMath::Min(m_fVelocityUp, vVelocity.z);
}

void ezJoltDefaultCharacterComponent::InteractWithSurfaces(const ContactPoint& contact, const Config& cfg)
{
  if (cfg.m_sGroundInteraction.IsEmpty())
  {
    m_fAccumulatedWalkDistance = 0;
    return;
  }

  const ezVec2 vIntendedWalkAmount = (cfg.m_vVelocity * GetUpdateTimeDelta()).GetAsVec2() + m_vAbsoluteRootMotion.GetAsVec2();

  const ezVec3 vOldPos = m_PreviousTransform.m_vPosition;
  const ezVec3 vNewPos = GetOwner()->GetGlobalPosition();

  m_fAccumulatedWalkDistance += ezMath::Min(vIntendedWalkAmount.GetLength(), (vNewPos - vOldPos).GetLength());

  if (m_fAccumulatedWalkDistance < cfg.m_fGroundInteractionDistanceThreshold)
    return;

  m_fAccumulatedWalkDistance = 0.0f;

  SpawnContactInteraction(contact, cfg.m_sGroundInteraction, m_hFallbackWalkSurface);
}

void ezJoltDefaultCharacterComponent::MoveHeadObject()
{
  ezGameObject* pHeadObject;
  if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  {
    m_fHeadTargetHeight = m_fHeadHeightOffset;

    if (IsCrouching())
    {
      m_fHeadTargetHeight -= (m_fCylinderHeightStand - m_fCylinderHeightCrouch);
    }

    ezVec3 pos = pHeadObject->GetLocalPosition();

    const float fTimeDiff = ezMath::Max(GetUpdateTimeDelta(), 0.005f); // prevent stuff from breaking at high frame rates
    const float fFactor = 1.0f - ezMath::Pow(0.001f, fTimeDiff);
    pos.z = ezMath::Lerp(pos.z, m_fHeadTargetHeight, fFactor);

    pHeadObject->SetLocalPosition(pos);
  }
}

void ezJoltDefaultCharacterComponent::DebugVisualizations()
{
  if (m_DebugFlags.IsSet(ezJoltCharacterDebugFlags::PrintState))
  {
    switch (GetJoltCharacter()->GetGroundState())
    {
      case JPH::CharacterBase::EGroundState::OnGround:
        ezDebugRenderer::DrawInfoText(GetWorld(), ezDebugTextPlacement::TopLeft, "JCC", "Jolt: On Ground", ezColor::Brown);
        break;
      case JPH::CharacterBase::EGroundState::InAir:
        ezDebugRenderer::DrawInfoText(GetWorld(), ezDebugTextPlacement::TopLeft, "JCC", "Jolt: In Air", ezColor::CornflowerBlue);
        break;
      case JPH::CharacterBase::EGroundState::NotSupported:
        ezDebugRenderer::DrawInfoText(GetWorld(), ezDebugTextPlacement::TopLeft, "JCC", "Jolt: Not Supported", ezColor::Yellow);
        break;
      case JPH::CharacterBase::EGroundState::OnSteepGround:
        ezDebugRenderer::DrawInfoText(GetWorld(), ezDebugTextPlacement::TopLeft, "JCC", "Jolt: Steep", ezColor::OrangeRed);
        break;
    }

    // const ezTransform newTransform = GetOwner()->GetGlobalTransform();
    // const float fDistTraveled = (m_PreviousTransform.m_vPosition - newTransform.m_vPosition).GetLength();
    // const float fSpeedTraveled = fDistTraveled * GetInverseUpdateTimeDelta();
    // const float fSpeedTraveledLateral = fDistTraveled * GetInverseUpdateTimeDelta();
    // ezDebugRenderer::DrawInfoText(GetWorld(), ezDebugTextPlacement::TopLeft, "JCC", ezFmt("Speed 1: {} m/s", fSpeedTraveled), ezColor::WhiteSmoke);
    // ezDebugRenderer::DrawInfoText(GetWorld(), ezDebugTextPlacement::TopLeft, "JCC", ezFmt("Speed 2: {} m/s", fSpeedTraveledLateral), ezColor::WhiteSmoke);
  }

  if (m_DebugFlags.IsSet(ezJoltCharacterDebugFlags::VisGroundContact))
  {
    ezVec3 gpos = ezJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundPosition());
    ezVec3 gnom = ezJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundNormal());

    if (!gnom.IsZero(0.01f))
    {
      ezQuat rot = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), gnom);

      ezDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.2f, ezColor::MakeZero(), ezColor::Aquamarine, ezTransform(gpos, rot));
    }
  }

  if (m_DebugFlags.IsSet(ezJoltCharacterDebugFlags::VisShape))
  {
    ezTransform shapeTrans = GetOwner()->GetGlobalTransform();

    shapeTrans.m_vPosition.z += GetCurrentCapsuleHeight() * 0.5f;

    ezDebugRenderer::DrawLineCapsuleZ(GetWorld(), GetCurrentCylinderHeight(), GetShapeRadius(), ezColor::CornflowerBlue, shapeTrans);
  }
}

void ezJoltDefaultCharacterComponent::CheckFeet()
{
  if (!cvar_JoltCcFootCheck)
  {
    // pretend we always touch the ground
    m_bFeetOnSolidGround = true;
    return;
  }

  if (m_fFootRadius <= 0 || m_fMaxStepDown <= 0.0f)
    return;

  m_bFeetOnSolidGround = false;

  ezTransform shapeTrans = GetOwner()->GetGlobalTransform();
  ezQuat shapeRot = ezQuat::MakeShortestRotation(ezVec3(0, 1, 0), ezVec3(0, 0, 1));

  const float radius = m_fFootRadius;
  const float halfHeight = ezMath::Max(0.01f, m_fMaxStepDown - radius);

  JPH::CapsuleShape shape(halfHeight, radius);

  ezHybridArray<ContactPoint, 32> contacts;
  CollectContacts(contacts, &shape, shapeTrans.m_vPosition, shapeRot, 0.01f);

  for (const auto& contact : contacts)
  {
    ezVec3 gpos = contact.m_vPosition;
    ezVec3 gnom = contact.m_vSurfaceNormal;

    ezColor color = ezColor::LightYellow;
    ezQuat rot;

    if (gnom.IsZero(0.01f))
    {
      rot = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), ezVec3::MakeAxisZ());
      color = ezColor::OrangeRed;
    }
    else
    {
      rot = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), gnom);

      if (gnom.Dot(ezVec3::MakeAxisZ()) > ezMath::Cos(ezAngle::MakeFromDegree(40)))
      {
        m_bFeetOnSolidGround = true;
        color = ezColor::GreenYellow;
      }
    }

    if (m_DebugFlags.IsAnySet(ezJoltCharacterDebugFlags::VisFootCheck))
    {
      ezDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.2f, ezColor::MakeZero(), color, ezTransform(gpos, rot));
    }
  }

  if (m_DebugFlags.IsAnySet(ezJoltCharacterDebugFlags::VisFootCheck))
  {
    ezDebugRenderer::DrawLineCapsuleZ(GetWorld(), halfHeight * 2.0f, radius, ezColor::YellowGreen, ezTransform(shapeTrans.m_vPosition));
  }
}

void ezJoltDefaultCharacterComponent::DetermineConfig(Config& out_inputs)
{
  // velocity
  {
    float fSpeed = 0;

    switch (GetGroundState())
    {
      case ezJoltDefaultCharacterComponent::GroundState::OnGround:
        fSpeed = m_fWalkSpeedStanding;

        if (m_uiIsCrouchingBit)
        {
          fSpeed = m_fWalkSpeedCrouching;
        }
        else if (m_uiInputRunBit)
        {
          fSpeed = m_fWalkSpeedRunning;
        }
        break;

      case ezJoltDefaultCharacterComponent::GroundState::Sliding:
        fSpeed = m_fWalkSpeedStanding;

        if (m_uiIsCrouchingBit)
        {
          fSpeed = m_fWalkSpeedCrouching;
        }
        break;

      case ezJoltDefaultCharacterComponent::GroundState::InAir:
        fSpeed = m_fAirSpeed;
        break;
    }

    out_inputs.m_vVelocity = GetOwner()->GetGlobalRotation() * m_vInputDirection.GetAsVec3(0) * fSpeed;
  }

  // ground interaction
  {
    switch (GetGroundState())
    {
      case ezJoltDefaultCharacterComponent::GroundState::OnGround:
        out_inputs.m_sGroundInteraction = (m_uiInputRunBit == 1) ? m_sWalkSurfaceInteraction : m_sWalkSurfaceInteraction; // TODO: run interaction
        out_inputs.m_fGroundInteractionDistanceThreshold = (m_uiInputRunBit == 1) ? m_fRunInteractionDistance : m_fWalkInteractionDistance;
        break;

      case ezJoltDefaultCharacterComponent::GroundState::Sliding:
        // TODO: slide interaction
        break;

      case GroundState::InAir:
        break;
    }
  }

  out_inputs.m_bAllowCrouch = true;
  out_inputs.m_bAllowJump = (GetGroundState() == GroundState::OnGround) && !IsCrouching() && m_bFeetOnSolidGround;
  out_inputs.m_bApplyGroundVelocity = true;
  out_inputs.m_fPushDownForce = GetMass();
  out_inputs.m_fMaxStepUp = (m_bFeetOnSolidGround && !out_inputs.m_vVelocity.IsZero()) ? m_fMaxStepUp : 0.0f;
  out_inputs.m_fMaxStepDown = ((GetGroundState() == GroundState::OnGround) || (GetGroundState() == GroundState::Sliding)) && m_bFeetOnSolidGround ? m_fMaxStepDown : 0.0f;
}

void ezJoltDefaultCharacterComponent::UpdateCharacter()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  m_PreviousTransform = GetOwner()->GetGlobalTransform();

  switch (GetJoltCharacter()->GetGroundState())
  {
    case JPH::CharacterBase::EGroundState::InAir:
    case JPH::CharacterBase::EGroundState::NotSupported:
      m_LastGroundState = GroundState::InAir;
      // TODO: filter out 'sliding' when touching a ceiling (should be 'in air')
      break;

    case JPH::CharacterBase::EGroundState::OnGround:
      m_LastGroundState = GroundState::OnGround;
      break;

    case JPH::CharacterBase::EGroundState::OnSteepGround:
      m_LastGroundState = GroundState::Sliding;
      break;
  }

  CheckFeet();

  Config cfg;
  DetermineConfig(cfg);

  ApplyCrouchState();

  if (m_uiInputJumpBit && cfg.m_bAllowJump)
  {
    m_fVelocityUp = m_fJumpImpulse;
    cfg.m_fMaxStepUp = 0;
    cfg.m_fMaxStepDown = 0;
  }

  ezVec3 vGroundVelocity = ezVec3::MakeZero();

  ContactPoint groundContact;
  {
    groundContact.m_vPosition = ezJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundPosition());
    groundContact.m_vContactNormal = ezJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundNormal());
    groundContact.m_vSurfaceNormal = groundContact.m_vContactNormal;
    groundContact.m_BodyID = GetJoltCharacter()->GetGroundBodyID();
    groundContact.m_SubShapeID = GetJoltCharacter()->GetGroundSubShapeID();

    /*vGroundVelocity =*/GetContactVelocityAndPushAway(groundContact, cfg.m_fPushDownForce);

    // TODO: on rotating surfaces I see the same error with this value and the one returned above
    vGroundVelocity = ezJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundVelocity());
    vGroundVelocity.z = 0.0f;

    if (!cfg.m_bApplyGroundVelocity)
      vGroundVelocity.SetZero();
  }

  const bool bWasOnGround = GetJoltCharacter()->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;

  if (bWasOnGround)
  {
    m_vVelocityLateral.SetZero();
  }

  // AIR: apply 'drag' to the lateral velocity
  m_vVelocityLateral *= ezMath::Pow(1.0f - m_fAirFriction, GetUpdateTimeDelta());

  ezVec3 vRootVelocity = GetInverseUpdateTimeDelta() * (GetOwner()->GetGlobalRotation() * m_vAbsoluteRootMotion);

  if (!m_vVelocityLateral.IsZero(ezMath::FloatEpsilon<float>()))
  {
    // remove the lateral velocity component from the root motion
    // to prevent root motion being amplified when both values are active
    ezVec3 vLatDir = m_vVelocityLateral.GetNormalized().GetAsVec3(0);
    float fProj = ezMath::Max(0.0f, vLatDir.Dot(vRootVelocity));
    vRootVelocity -= vLatDir * fProj;
  }

  ezVec3 vVelocityToApply = cfg.m_vVelocity + vGroundVelocity;
  vVelocityToApply += m_vVelocityLateral.GetAsVec3(0);
  vVelocityToApply += vRootVelocity;
  vVelocityToApply.z = m_fVelocityUp;

  RawMoveWithVelocity(vVelocityToApply, cfg.m_fMaxStepUp, cfg.m_fMaxStepDown);

  if (!cfg.m_sGroundInteraction.IsEmpty())
  {
    if (groundContact.m_vContactNormal.IsValid() && !groundContact.m_vContactNormal.IsZero(0.001f))
    {
      // TODO: sometimes the CC reports contacts with zero normals
      InteractWithSurfaces(groundContact, cfg);
    }
  }

  // retrieve the actual up velocity
  float groundVerticalVelocity = GetJoltCharacter()->GetGroundVelocity().GetZ();
  if (GetJoltCharacter()->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround // If on ground
      && (m_fVelocityUp - groundVerticalVelocity) < 0.1f)                                   // And not moving away from ground
  {
    m_fVelocityUp = groundVerticalVelocity;
  }

  if (bWasOnGround)
  {
    StoreLateralVelocity();
  }
  else
  {
    ClampLateralVelocity();
    ClampUpVelocity();
  }

  m_fVelocityUp += GetUpdateTimeDelta() * pModule->GetCharacterGravity().z;

  ApplyRotationZ();

  MoveHeadObject();

  DebugVisualizations();

  ResetInputState();
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Character_Implementation_JoltDefaultCharacterComponent);
