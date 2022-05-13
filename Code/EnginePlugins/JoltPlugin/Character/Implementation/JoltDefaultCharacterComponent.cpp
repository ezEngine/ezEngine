#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/Stats.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <JoltPlugin/Character/JoltDefaultCharacterComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltDefaultCharacterComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ShapeRadius", m_fShapeRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f)),
    EZ_MEMBER_PROPERTY("CrouchHeight", m_fCylinderHeightCrouch)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("StandHeight", m_fCylinderHeightStand)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("WalkSpeedCrouching", m_fWalkSpeedCrouching)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("WalkSpeedStanding", m_fWalkSpeedStanding)->AddAttributes(new ezDefaultValueAttribute(2.5f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("WalkSpeedRunning", m_fWalkSpeedRunning)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("AirSpeed", m_fAirSpeed)->AddAttributes(new ezDefaultValueAttribute(2.5f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("AirFriction", m_fAirFriction)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("MaxStepHeight", m_fMaxStepHeight)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("JumpImpulse", m_fJumpImpulse)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, 1000.0f)),
    EZ_MEMBER_PROPERTY("RotateSpeed", m_RotateSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90.0f)), new ezClampValueAttribute(ezAngle::Degree(1.0f), ezAngle::Degree(360.0f))),
    EZ_ACCESSOR_PROPERTY("WalkSurfaceInteraction", GetWalkSurfaceInteraction, SetWalkSurfaceInteraction)->AddAttributes(new ezDynamicStringEnumAttribute("SurfaceInteractionTypeEnum"), new ezDefaultValueAttribute(ezStringView("Footstep"))),
    EZ_MEMBER_PROPERTY("WalkInteractionDistance", m_fWalkInteractionDistance)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("RunInteractionDistance", m_fRunInteractionDistance)->AddAttributes(new ezDefaultValueAttribute(3.0f)),
    EZ_ACCESSOR_PROPERTY("FallbackWalkSurface", GetFallbackWalkSurfaceFile, SetFallbackWalkSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
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
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezJoltDefaultCharacterComponent::ezJoltDefaultCharacterComponent() = default;
ezJoltDefaultCharacterComponent::~ezJoltDefaultCharacterComponent() = default;

void ezJoltDefaultCharacterComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, GetShapeRadius()), GetShapeRadius()), ezInvalidSpatialDataCategory);
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, GetCurrentTotalHeight() - GetShapeRadius()), GetShapeRadius()), ezInvalidSpatialDataCategory);
}

void ezJoltDefaultCharacterComponent::OnApplyRootMotion(ezMsgApplyRootMotion& msg)
{
  m_vAbsoluteRootMotion = msg.m_vTranslation;
  m_InputRotateZ += msg.m_RotationZ;
}

void ezJoltDefaultCharacterComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_RotateSpeed;
  s << m_fShapeRadius;
  s << m_fCylinderHeightCrouch;
  s << m_fCylinderHeightStand;
  s << m_fWalkSpeedCrouching;
  s << m_fWalkSpeedStanding;
  s << m_fWalkSpeedRunning;
  s << m_fMaxStepHeight;
  s << m_fJumpImpulse;
  s << m_sWalkSurfaceInteraction;
  s << m_fWalkInteractionDistance;
  s << m_fRunInteractionDistance;
  s << m_hFallbackWalkSurface;
  s << m_fAirFriction;
  s << m_fAirSpeed;

  stream.WriteGameObjectHandle(m_hHeadObject);
}

void ezJoltDefaultCharacterComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_RotateSpeed;
  s >> m_fShapeRadius;
  s >> m_fCylinderHeightCrouch;
  s >> m_fCylinderHeightStand;
  s >> m_fWalkSpeedCrouching;
  s >> m_fWalkSpeedStanding;
  s >> m_fWalkSpeedRunning;
  s >> m_fMaxStepHeight;
  s >> m_fJumpImpulse;
  s >> m_sWalkSurfaceInteraction;
  s >> m_fWalkInteractionDistance;
  s >> m_fRunInteractionDistance;
  s >> m_hFallbackWalkSurface;
  s >> m_fAirFriction;
  s >> m_fAirSpeed;

  m_hHeadObject = stream.ReadGameObjectHandle();

  ResetInternalState();
}

void ezJoltDefaultCharacterComponent::ResetInternalState()
{
  m_fShapeRadius = ezMath::Clamp(m_fShapeRadius, 0.05f, 5.0f);
  m_fCylinderHeightCrouch = ezMath::Max(m_fCylinderHeightCrouch, 0.01f);
  m_fCylinderHeightStand = ezMath::Max(m_fCylinderHeightStand, m_fCylinderHeightCrouch);
  m_fMaxStepHeight = ezMath::Clamp(m_fMaxStepHeight, 0.0f, m_fCylinderHeightStand);

  m_fNextCylinderHeight = m_fCylinderHeightStand;
  m_fCurrentCylinderHeight = m_fNextCylinderHeight;

  m_vVelocityLateral.SetZero();
  m_fVelocityUp = 0;

  m_fAccumulatedWalkDistance = 0;
}

void ezJoltDefaultCharacterComponent::ResetInputState()
{
  m_InputDirection.SetZero();
  m_InputRotateZ = ezAngle();
  m_InputCrouchBit = 0;
  m_InputRunBit = 0;
  m_InputJumpBit = 0;
  m_vAbsoluteRootMotion.SetZero();
}

void ezJoltDefaultCharacterComponent::SetInputState(ezMsgMoveCharacterController& msg)
{
  const float fDistanceToMove = ezMath::Max(ezMath::Abs((float)(msg.m_fMoveForwards - msg.m_fMoveBackwards)), ezMath::Abs((float)(msg.m_fStrafeRight - msg.m_fStrafeLeft)));

  m_InputDirection += ezVec2((float)(msg.m_fMoveForwards - msg.m_fMoveBackwards), (float)(msg.m_fStrafeRight - msg.m_fStrafeLeft));
  m_InputDirection.NormalizeIfNotZero(ezVec2::ZeroVector()).IgnoreResult();
  m_InputDirection *= fDistanceToMove;

  m_InputRotateZ += m_RotateSpeed * (float)(msg.m_fRotateRight - msg.m_fRotateLeft);

  if (msg.m_bRun)
  {
    m_InputRunBit = 1;
  }

  if (msg.m_bJump)
  {
    m_InputJumpBit = 1;
  }

  if (msg.m_bCrouch)
  {
    m_InputCrouchBit = 1;
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

float ezJoltDefaultCharacterComponent::GetCurrentTotalHeight() const
{
  return m_fMaxStepHeight + GetCurrentCapsuleHeight();
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
  // TODO: remove query shape etc

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
  up.mPosition = JPH::Vec3(0, 0, fTotalCapsuleHeight * 0.5f + m_fMaxStepHeight);
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

  // if (m_bQueryShape)
  {
    const ezSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    JPH::BodyCreationSettings bodyCfg;
    ezJoltUserData* pUserData = nullptr;
    m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
    pUserData->Init(this);

    bodyCfg.SetShape(MakeNextCharacterShape().GetPtr()); // TODO: shape without step offset
    bodyCfg.mPosition = ezJoltConversionUtils::ToVec3(trans.m_Position);
    bodyCfg.mRotation = ezJoltConversionUtils::ToQuat(trans.m_Rotation);
    bodyCfg.mMotionType = JPH::EMotionType::Static;
    bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Character);
    bodyCfg.mMotionQuality = JPH::EMotionQuality::Discrete;
    // bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
    // bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // the group filter is only needed for objects constrained via joints
    bodyCfg.mUserData = reinterpret_cast<ezUInt64>(pUserData);

    JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
    m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

    pModule->QueueBodyToAdd(pBody);
  }
}

void ezJoltDefaultCharacterComponent::UpdateCharacter()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  m_PreviousTransform = GetOwner()->GetGlobalTransform();

  switch (DetermineCurrentState(m_LastGroundState == GroundState::OnGround))
  {
    case ezJoltDefaultCharacterComponent::GroundState::OnGround:
      Update_OnGround();
      break;
    case ezJoltDefaultCharacterComponent::GroundState::Sliding:
      Update_Sliding();
      break;
    case ezJoltDefaultCharacterComponent::GroundState::Jumping:
    case ezJoltDefaultCharacterComponent::GroundState::Falling:
      Update_InAir();
      break;
  }

  ApplyRotationZ();

  VisualizeShape();

  if (m_DebugFlags.IsSet(ezJoltCharacterDebugFlags::VisContacts))
  {
    VisualizeContacts(m_CurrentContacts, ezColor::LightPink);
  }

  ClampUpVelocity();

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

  // if query shape
  {
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    JPH::BodyID bodyId(m_uiJoltBodyID);

    if (!bodyId.IsInvalid())
    {
      ezVec3 pos = GetOwner()->GetGlobalPosition();
      ezQuat rot = GetOwner()->GetGlobalRotation();

      pos.z -= m_fMaxStepHeight; // TODO shape hack

      pBodies->SetPositionAndRotation(bodyId, ezJoltConversionUtils::ToVec3(pos), ezJoltConversionUtils::ToQuat(rot).Normalized(), JPH::EActivation::DontActivate);
    }
  }

  ResetInputState();
}

void ezJoltDefaultCharacterComponent::Update_OnGround()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  if (m_DebugFlags.IsSet(ezJoltCharacterDebugFlags::PrintState))
  {
    ezDebugRenderer::DrawInfoText(GetWorld(), ezDebugRenderer::ScreenPlacement::TopLeft, "JCC", "State: On Ground", ezColor::Brown);
  }

  m_LastGroundState = GroundState::OnGround;

  ApplyCrouchState();

  const ezVec3 vInputVelocity = ComputeInputVelocity_OnGround();

  // touching the ground means we can't move downwards any further
  m_fVelocityUp = ezMath::Max(m_fVelocityUp, 0.0f);

  if (m_InputJumpBit && !m_IsCrouchingBit)
  {
    m_fVelocityUp = m_fJumpImpulse;
    m_LastGroundState = GroundState::Jumping;
  }

  const ezTransform ownTransform = GetOwner()->GetLocalTransform();

  ContactPoint groundContact;
  auto newGroundState = DetermineGroundState(groundContact, ownTransform.m_vPosition + GetUpdateTimeDelta() * vInputVelocity, m_fMaxStepHeight, (m_LastGroundState == GroundState::OnGround) ? m_fMaxStepHeight : 0.0f, GetShapeRadius());

  const ezVec3 vGroundVelocity = GetContactVelocityAndPushAway(groundContact, GetMass());

  ezVec3 vVelocityToApply = vInputVelocity + vGroundVelocity;
  vVelocityToApply.z = m_fVelocityUp;

  if (newGroundState == GroundState::OnGround)
  {
    // only step up or down, if we are firmly on the ground (not even sliding
    AdjustVelocityToStepUpOrDown(vVelocityToApply.z, groundContact);
  }

  const ezVec3 vPrevPos = GetOwner()->GetGlobalPosition();
  RawMoveWithVelocity(vVelocityToApply);
  RawMoveIntoDirection(GetOwner()->GetGlobalRotation() * m_vAbsoluteRootMotion);

  InteractWithSurfaces((GetUpdateTimeDelta() * vInputVelocity.GetAsVec2()) + m_vAbsoluteRootMotion.GetAsVec2(), vPrevPos, GroundState::OnGround, groundContact);

  StoreLateralVelocity();
}

void ezJoltDefaultCharacterComponent::Update_InAir()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  if (m_DebugFlags.IsSet(ezJoltCharacterDebugFlags::PrintState))
  {
    ezDebugRenderer::DrawInfoText(GetWorld(), ezDebugRenderer::ScreenPlacement::TopLeft, "JCC", "State: In Air", ezColor::CornflowerBlue);
  }

  if (m_LastGroundState != GroundState::Jumping || m_fVelocityUp < 0.0f)
    m_LastGroundState = GroundState::Falling;

  // when falling etc, always stand up
  // m_InputCrouchBit = 0;
  ApplyCrouchState();

  const ezVec3 vInputVelocity = ComputeInputVelocity_InAir();

  m_fVelocityUp += GetUpdateTimeDelta() * pModule->GetCharacterGravity().z;

  // apply 'drag' to the lateral velocity
  m_vVelocityLateral *= ezMath::Pow(1.0f - m_fAirFriction, GetUpdateTimeDelta());

  ezVec3 vVelocityToApply = vInputVelocity + m_vVelocityLateral.GetAsVec3(0);
  vVelocityToApply.z = m_fVelocityUp;

  RawMoveWithVelocity(vVelocityToApply);
  RawMoveIntoDirection(GetOwner()->GetGlobalRotation() * m_vAbsoluteRootMotion);

  ClampLateralVelocity();
}

void ezJoltDefaultCharacterComponent::Update_Sliding()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  if (m_DebugFlags.IsSet(ezJoltCharacterDebugFlags::PrintState))
  {
    ezDebugRenderer::DrawInfoText(GetWorld(), ezDebugRenderer::ScreenPlacement::TopLeft, "JCC", "State: Sliding", ezColor::DarkOrange);
  }

  m_LastGroundState = GroundState::Sliding;

  // when sliding, always stand up
  // m_InputCrouchBit = 0;
  ApplyCrouchState();

  const ezVec3 vInputVelocity = ComputeInputVelocity_OnGround();

  // TODO: if sliding -> slide speed and direction depending on slope
  m_fVelocityUp += GetUpdateTimeDelta() * pModule->GetCharacterGravity().z;

  const ezTransform ownTransform = GetOwner()->GetLocalTransform();

  ContactPoint groundContact;
  DetermineGroundState(groundContact, ownTransform.m_vPosition + GetUpdateTimeDelta() * vInputVelocity, m_fMaxStepHeight, 0.0f, GetShapeRadius());

  const ezVec3 vGroundVelocity = GetContactVelocityAndPushAway(groundContact, GetMass());

  ezVec3 vVelocityToApply = vInputVelocity + vGroundVelocity;
  vVelocityToApply.z = m_fVelocityUp;

  const ezVec3 vPrevPos = GetOwner()->GetGlobalPosition();
  RawMoveWithVelocity(vVelocityToApply);
  RawMoveIntoDirection(GetOwner()->GetGlobalRotation() * m_vAbsoluteRootMotion);

  InteractWithSurfaces(GetUpdateTimeDelta() * vInputVelocity.GetAsVec2(), vPrevPos, GroundState::Sliding, groundContact);

  StoreLateralVelocity();
}

void ezJoltDefaultCharacterComponent::ApplyRotationZ()
{
  if (m_InputRotateZ.GetRadian() == 0.0f)
    return;

  ezQuat qRotZ;
  qRotZ.SetFromAxisAndAngle(ezVec3(0, 0, 1), m_InputRotateZ);
  m_InputRotateZ.SetRadian(0.0);

  GetOwner()->SetGlobalRotation(qRotZ * GetOwner()->GetGlobalRotation());
}

ezJoltDefaultCharacterComponent::GroundState ezJoltDefaultCharacterComponent::DetermineGroundState(ContactPoint& out_Contact, const ezVec3& vFootPosition, float fAllowedStepUp, float fAllowedStepDown, float fCylinderRadius) const
{
  out_Contact.m_BodyID = JPH::BodyID();

  const JPH::SphereShape shape(fCylinderRadius);
  ezQuat qRotYtoZ;
  qRotYtoZ.SetShortestRotation(ezVec3::UnitYAxis(), ezVec3::UnitZAxis());

  const ezVec3 vStartSweep = vFootPosition + ezVec3(0, 0, fCylinderRadius + fAllowedStepUp);
  const ezVec3 vEndSweep = vFootPosition + ezVec3(0, 0, fCylinderRadius - fAllowedStepDown - 0.01f);

  ezHybridArray<ContactPoint, 32> groundContacts;
  CollectCastContacts(groundContacts, &shape, vStartSweep, qRotYtoZ, vEndSweep - vStartSweep);

  if (m_DebugFlags.IsSet(ezJoltCharacterDebugFlags::VisCasts))
  {
    ezDebugRenderer::DrawLineCapsuleZ(GetWorld(), (vEndSweep - vStartSweep).GetLength(), shape.GetRadius(), ezColor::PaleTurquoise, ezTransform(ezMath::Lerp(vStartSweep, vEndSweep, 0.5f)));
  }

  ezUInt32 uiBestGroundContact = ezInvalidIndex;
  auto contactFlags = ClassifyContacts(groundContacts, GetMaxClimbingSlope(), vStartSweep, &uiBestGroundContact);

  GroundState state = GroundState::Falling;

  if (contactFlags.IsSet(ShapeContacts::FlatGround))
  {
    state = GroundState::OnGround;
    out_Contact = groundContacts[uiBestGroundContact];
  }
  else if (contactFlags.IsSet(ShapeContacts::SteepGround))
  {
    state = GroundState::Sliding;
    out_Contact = groundContacts[uiBestGroundContact];
  }

  if (!out_Contact.m_BodyID.IsInvalid() && m_DebugFlags.IsSet(ezJoltCharacterDebugFlags::VisGroundContact))
  {
    const ezColor color = (state == GroundState::OnGround) ? ezColor::RosyBrown : ezColor::OrangeRed;

    ezTransform trans;
    trans.m_vPosition = out_Contact.m_vPosition;
    trans.m_qRotation.SetShortestRotation(ezVec3::UnitXAxis(), out_Contact.m_vContactNormal);
    trans.m_vScale.Set(1.0f);
    ezDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.1f, ezColor::ZeroColor(), color, trans);
  }

  return state;
}

ezJoltDefaultCharacterComponent::GroundState ezJoltDefaultCharacterComponent::DetermineCurrentState(bool bAllowStepDown)
{
  if (m_LastGroundState == GroundState::Jumping && m_fVelocityUp > 0)
    return GroundState::Jumping;

  const ezVec3 vFootPosition = GetOwner()->GetGlobalPosition();

  const float fTotalCharacterHeight = GetCurrentTotalHeight();

  const float fQueryDownDistance = bAllowStepDown ? m_fMaxStepHeight : 0.0f;
  const float fQueryRadius = GetShapeRadius();
  const float fQueryShapeHeight = fTotalCharacterHeight + fQueryDownDistance;
  const float fQueryCylinderHeight = fQueryShapeHeight - 2.0f * GetShapeRadius();

  const JPH::CapsuleShape shape(fQueryCylinderHeight * 0.5f, fQueryRadius);
  ezQuat qRotYtoZ;
  qRotYtoZ.SetShortestRotation(ezVec3::UnitYAxis(), ezVec3::UnitZAxis());

  const ezVec3 vCenterPos = vFootPosition + ezVec3(0, 0, fQueryShapeHeight * 0.5f - fQueryDownDistance);
  CollectContacts(m_CurrentContacts, &shape, vCenterPos, qRotYtoZ, 0.0f);

  if (m_DebugFlags.IsSet(ezJoltCharacterDebugFlags::VisCasts))
  {
    ezDebugRenderer::DrawLineCapsuleZ(GetWorld(), shape.GetHalfHeightOfCylinder() * 2.0f, shape.GetRadius(), ezColor::AntiqueWhite, ezTransform(vCenterPos));
  }

  auto contactFlags = ClassifyContacts(m_CurrentContacts, GetMaxClimbingSlope(), vCenterPos, nullptr);

  GroundState state = GroundState::Falling;

  if (contactFlags.IsSet(ShapeContacts::FlatGround))
  {
    state = GroundState::OnGround;
  }
  else if (contactFlags.IsSet(ShapeContacts::SteepGround))
  {
    state = GroundState::Sliding;
  }

  return state;
}

ezVec3 ezJoltDefaultCharacterComponent::ComputeInputVelocity_OnGround() const
{
  float fSpeed = m_fWalkSpeedStanding;

  if (m_IsCrouchingBit)
  {
    fSpeed = m_fWalkSpeedCrouching;
  }
  else if (m_InputRunBit)
  {
    fSpeed = m_fWalkSpeedRunning;
  }

  return GetOwner()->GetGlobalRotation() * m_InputDirection.GetAsVec3(0) * fSpeed;
}

ezVec3 ezJoltDefaultCharacterComponent::ComputeInputVelocity_InAir() const
{
  return GetOwner()->GetGlobalRotation() * m_InputDirection.GetAsVec3(0) * m_fAirSpeed;
}

void ezJoltDefaultCharacterComponent::AdjustVelocityToStepUpOrDown(float& fVelocityUp, const ContactPoint& groundContact) const
{
  // don't do anything if we are currently jumping or falling
  if (fVelocityUp != 0.0f)
    return;

  const float fGroundConnectDist = ezMath::Clamp(groundContact.m_vPosition.z - GetOwner()->GetGlobalPosition().z, -m_fMaxStepHeight, +m_fMaxStepHeight);

  fVelocityUp += (fGroundConnectDist * GetInverseUpdateTimeDelta()) * 0.25f;
}

void ezJoltDefaultCharacterComponent::ApplyCrouchState()
{
  if (m_InputCrouchBit == m_IsCrouchingBit)
    return;

  m_fNextCylinderHeight = m_InputCrouchBit ? m_fCylinderHeightCrouch : m_fCylinderHeightStand;

  if (TryChangeShape(MakeNextCharacterShape().GetPtr()).Succeeded())
  {
    m_IsCrouchingBit = m_InputCrouchBit;
    m_fCurrentCylinderHeight = m_fNextCylinderHeight;
  }
}

void ezJoltDefaultCharacterComponent::SetFallbackWalkSurfaceFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hFallbackWalkSurface = ezResourceManager::LoadResource<ezSurfaceResource>(szFile);
  }

  if (m_hFallbackWalkSurface.IsValid())
    ezResourceManager::PreloadResource(m_hFallbackWalkSurface);
}

const char* ezJoltDefaultCharacterComponent::GetFallbackWalkSurfaceFile() const
{
  if (!m_hFallbackWalkSurface.IsValid())
    return "";

  return m_hFallbackWalkSurface.GetResourceID();
}

void ezJoltDefaultCharacterComponent::InteractWithSurfaces(const ezVec2& vWalkAmount, const ezVec3& vStartPos, GroundState groundState, const ContactPoint& contact)
{
  if (m_sWalkSurfaceInteraction.IsEmpty())
    return;

  if (groundState == GroundState::Sliding)
  {
    // TODO: sliding interaction (could reuse feature for other surface interactions)
    // const ezVec3 vNewPos = GetOwner()->GetGlobalPosition();
    // m_fAccumulatedWalkDistance += (vNewPos - vStartPos).GetLength();
    // SpawnContactInteraction(m_GroundContact, m_sWalkSurfaceInteraction, m_hFallbackWalkSurface);
  }
  else if (groundState == GroundState::OnGround)
  {
    const ezVec3 vNewPos = GetOwner()->GetGlobalPosition();
    m_fAccumulatedWalkDistance += ezMath::Min(vWalkAmount.GetLength(), (vNewPos - vStartPos).GetLength());

    const bool bShouldInteract = (m_InputRunBit == 0 && m_fAccumulatedWalkDistance >= m_fWalkInteractionDistance) || (m_InputRunBit && m_fAccumulatedWalkDistance >= m_fRunInteractionDistance);

    if (!bShouldInteract)
      return;

    m_fAccumulatedWalkDistance = 0.0f;

    SpawnContactInteraction(contact, m_sWalkSurfaceInteraction, m_hFallbackWalkSurface);
  }
  else
    m_fAccumulatedWalkDistance = 0;
}

void ezJoltDefaultCharacterComponent::ClampUpVelocity()
{
  const ezVec3 endPosition = GetOwner()->GetGlobalPosition();

  const float fVelUp = (endPosition.z - m_PreviousTransform.m_vPosition.z) * GetInverseUpdateTimeDelta();
  m_fVelocityUp = ezMath::Min(m_fVelocityUp, fVelUp);
}

void ezJoltDefaultCharacterComponent::VisualizeShape()
{
  if (!m_DebugFlags.IsSet(ezJoltCharacterDebugFlags::VisShape))
    return;

  ezTransform shapeTrans = GetOwner()->GetGlobalTransform();

  shapeTrans.m_vPosition.z += m_fMaxStepHeight;
  shapeTrans.m_vPosition.z += GetCurrentCapsuleHeight() * 0.5f;

  ezDebugRenderer::DrawLineCapsuleZ(GetWorld(), GetCurrentCylinderHeight(), GetShapeRadius(), ezColor::CornflowerBlue, shapeTrans);
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

    m_vVelocityLateral.SetLength(fSpeedAlongRealDir).IgnoreResult();
  }
  else
    m_vVelocityLateral.SetZero();
}
