#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Character/JoltFpsCharacterControllerComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <Physics/Body/BodyLock.h>
#include <Physics/Collision/CollideShape.h>
#include <Physics/Collision/Shape/CylinderShape.h>
#include <Physics/Collision/Shape/Shape.h>
#include <Physics/Collision/ShapeCast.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

ezJoltFpsCharacterControllerComponentManager::ezJoltFpsCharacterControllerComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezJoltFpsCharacterControllerComponent, ezBlockStorageType::Compact>(pWorld)
{
}

ezJoltFpsCharacterControllerComponentManager::~ezJoltFpsCharacterControllerComponentManager() = default;

void ezJoltFpsCharacterControllerComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltFpsCharacterControllerComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PostAsync;

  this->RegisterUpdateFunction(desc);
}

void ezJoltFpsCharacterControllerComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ezJoltFpsCharacterControllerComponent* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezJoltCCDebugFlags, 1)
  EZ_BITFLAGS_CONSTANTS(ezJoltCCDebugFlags::VisQueryShape, ezJoltCCDebugFlags::VisContactPoints, ezJoltCCDebugFlags::VisGroundContact, ezJoltCCDebugFlags::VisVelocity)
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_COMPONENT_TYPE(ezJoltFpsCharacterControllerComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CapsuleHeight", m_fCapsuleHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("CapsuleRadius", m_fCapsuleRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.1f, 5.0f)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("Mass", m_fMass)->AddAttributes(new ezDefaultValueAttribute(50.0f), new ezClampValueAttribute(0.1f, 10000.0f)),
    EZ_MEMBER_PROPERTY("RotateSpeed", m_RotateSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90.0f)), new ezClampValueAttribute(ezAngle::Degree(1.0f), ezAngle::Degree(360.0f))),
    EZ_MEMBER_PROPERTY("WalkSpeed", m_fWalkSpeed)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("RunSpeed", m_fRunSpeed)->AddAttributes(new ezDefaultValueAttribute(15.0f), new ezClampValueAttribute(0.01f, 20.0f)),
    //EZ_MEMBER_PROPERTY("AirSpeed", m_fAirSpeed)->AddAttributes(new ezDefaultValueAttribute(2.5f), new ezClampValueAttribute(0.01f, 20.0f)),
    //EZ_MEMBER_PROPERTY("AirFriction", m_fAirFriction)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("CrouchHeight", m_fCrouchHeight)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("CrouchSpeed", m_fCrouchSpeed)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(0.01f, 20.0f)),
    EZ_MEMBER_PROPERTY("JumpImpulse", m_fJumpImpulse)->AddAttributes(new ezDefaultValueAttribute(6.0f), new ezClampValueAttribute(0.0f, 50.0f)),
    //EZ_MEMBER_PROPERTY("PushingForce", m_fPushingForce)->AddAttributes(new ezDefaultValueAttribute(500.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    //EZ_ACCESSOR_PROPERTY("WalkSurfaceInteraction", GetWalkSurfaceInteraction, SetWalkSurfaceInteraction)->AddAttributes(new ezDynamicStringEnumAttribute("SurfaceInteractionTypeEnum"), new ezDefaultValueAttribute(ezStringView("Footstep"))),
    //EZ_MEMBER_PROPERTY("WalkInteractionDistance", m_fWalkInteractionDistance)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    //EZ_MEMBER_PROPERTY("RunInteractionDistance", m_fRunInteractionDistance)->AddAttributes(new ezDefaultValueAttribute(3.0f)),
    //EZ_ACCESSOR_PROPERTY("FallbackWalkSurface", GetFallbackWalkSurfaceFile, SetFallbackWalkSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
    //EZ_ACCESSOR_PROPERTY("HeadObject", DummyGetter, SetHeadObjectReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_MEMBER_PROPERTY("MaxStepHeight", m_fMaxStepHeight)->AddAttributes(new ezDefaultValueAttribute(0.3f), new ezClampValueAttribute(0.0f, 5.0f)),
    EZ_MEMBER_PROPERTY("MaxSlopeAngle", m_MaxClimbingSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(40.0f)), new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(80.0f))),
    EZ_MEMBER_PROPERTY("ForceSlopeSliding", m_bForceSlopeSliding)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_BITFLAGS_MEMBER_PROPERTY("DebugFlags", ezJoltCCDebugFlags , m_DebugFlags),

  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgApplyRootMotion, OnApplyRootMotion),
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Character"),
    new ezCapsuleVisualizerAttribute("CapsuleHeight", "CapsuleRadius", ezColor::MediumVioletRed, nullptr, ezVisualizerAnchor::NegZ),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetCurrentHeightValue),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezJoltFpsCharacterControllerComponent::ezJoltFpsCharacterControllerComponent()
{
  // m_sWalkSurfaceInteraction.Assign("Footstep");
}

void ezJoltFpsCharacterControllerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fCapsuleHeight;
  s << m_fCapsuleRadius;
  s << m_uiCollisionLayer;
  s << m_fMass;
  s << m_fWalkSpeed;
  s << m_fRunSpeed;
  // s << m_fAirSpeed;
  // s << m_fAirFriction;
  s << m_RotateSpeed;
  s << m_fJumpImpulse;
  // s << m_fPushingForce;
  // s << m_sWalkSurfaceInteraction;
  // s << m_fWalkInteractionDistance;
  // s << m_fRunInteractionDistance;
  // s << m_hFallbackWalkSurface;
  s << m_fCrouchSpeed;
  s << m_fCrouchHeight;
  // stream.WriteGameObjectHandle(m_hHeadObject);
  s << m_DebugFlags;
  s << m_fMaxStepHeight;
}

void ezJoltFpsCharacterControllerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fCapsuleHeight;
  s >> m_fCapsuleRadius;
  s >> m_uiCollisionLayer;
  s >> m_fMass;
  s >> m_fWalkSpeed;
  s >> m_fRunSpeed;
  // s >> m_fAirSpeed;
  // s >> m_fAirFriction;
  s >> m_RotateSpeed;
  s >> m_fJumpImpulse;
  // s >> m_fPushingForce;
  // s >> m_sWalkSurfaceInteraction;
  // s >> m_fWalkInteractionDistance;
  // s >> m_fRunInteractionDistance;
  // s >> m_hFallbackWalkSurface;
  s >> m_fCrouchSpeed;
  s >> m_fCrouchHeight;
  // m_hHeadObject = stream.ReadGameObjectHandle();
  s >> m_DebugFlags;
  s >> m_fMaxStepHeight;
}

void ezJoltFpsCharacterControllerComponent::RawMove(const ezVec3& vMoveDeltaGlobal)
{
}

void ezJoltFpsCharacterControllerComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void ezJoltFpsCharacterControllerComponent::OnDeactivated()
{
  if (m_pController != nullptr)
  {
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

    m_pController->RemoveFromPhysicsSystem();
    m_pController = nullptr;

    pModule->DeallocateUserData(m_uiUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }

  SUPER::OnDeactivated();
}

void ezJoltFpsCharacterControllerComponent::OnSimulationStarted()
{
  ResetInputState();

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  ezJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  m_IsCrouchingBit = 0;

  m_fCurrentHeightValue = m_fCapsuleHeight;

  m_uiObjectFilterID = pModule->CreateObjectFilterID();

  {
    const ezTransform trans = GetOwner()->GetGlobalTransform();

    JPH::CharacterSettings opt;
    opt.mMass = m_fMass;
    opt.mMaxSlopeAngle = m_MaxClimbingSlope.GetRadian();
    opt.mLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Character);
    opt.mShape = CreateCharacterShape(GetCurrentHeightValue());
    opt.mGravityFactor = 0;
    opt.mFriction = 0.0f;

    m_pController = new JPH::Character(&opt, ezJoltConversionUtils::ToVec3(trans.m_vPosition), ezJoltConversionUtils::ToQuat(trans.m_qRotation), 0, pModule->GetJoltSystem());

    {
      JPH::BodyLockWrite bodyLock(pModule->GetJoltSystem()->GetBodyLockInterface(), m_pController->GetBodyID());
      bodyLock.GetBody().GetCollisionGroup().SetGroupID(m_uiObjectFilterID);
    }

    m_pController->AddToPhysicsSystem(JPH::EActivation::Activate);
  }

  // ezGameObject* pHeadObject;
  // if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  //{
  //   m_fHeadHeightOffset = pHeadObject->GetLocalPosition().z;
  //   m_fHeadTargetHeight = m_fHeadHeightOffset;
  // }
}

ezVec3 ezJoltFpsCharacterControllerComponent::ComputeInputVelocity() const
{
  float fSpeed = m_fWalkSpeed;

  if (m_IsCrouchingBit)
  {
    fSpeed = m_fCrouchSpeed;
  }
  else if (m_InputRunBit)
  {
    fSpeed = m_fRunSpeed;
  }

  return GetOwner()->GetGlobalRotation() * m_vInputDirection * fSpeed;
}

void ezJoltFpsCharacterControllerComponent::Update()
{
  if (m_pController == nullptr)
    return;

  ApplyPhysicsTransform();
  ApplyCrouchState();

  ezHybridArray<ezDebugRenderer::Line, 16> lines;
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  const float tDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

  const ezTransform ownTransform = GetOwner()->GetGlobalTransform();

  const ezVec3 vInputVelocity = ComputeInputVelocity();
  const ezVec3 vClampedVelocity = RestrictMovementByGeometry(vInputVelocity, GetCurrentTotalHeight(), m_fCapsuleRadius);

  if (m_DebugFlags.IsSet(ezJoltCCDebugFlags::VisVelocity))
  {
    const ezVec3 vCenterPos = ownTransform.m_vPosition + ezVec3(0, 0, m_fCapsuleRadius);
    lines.PushBack(ezDebugRenderer::Line(vCenterPos, vCenterPos + vInputVelocity, ezColor::LightSkyBlue));
    lines.PushBack(ezDebugRenderer::Line(vCenterPos, vCenterPos + vClampedVelocity, ezColor::LimeGreen));
  }

  m_GroundState = DetermineGroundState(m_GroundContact, ownTransform.m_vPosition + tDiff * vClampedVelocity, m_fMaxStepHeight, (m_fVelocityUp == 0.0f) ? m_fMaxStepHeight : 0.0f, m_fCapsuleRadius * 0.8f);

  if (m_GroundState == GroundState::OnGround)
  {
    m_fVelocityUp = ezMath::Max(m_fVelocityUp, 0.0f);

    if (m_InputJumpBit)
    {
      m_fVelocityUp = m_fJumpImpulse;
      m_GroundState = GroundState::InAir;
    }
  }
  else
  {
    // TODO: if sliding -> slide speed depending on slope
    m_fVelocityUp += tDiff * pModule->GetCharacterGravity().z;
  }

  const ezVec3 vGroundVelocity = GetGroundVelocityAndPushDown(m_GroundContact, pModule->GetJoltSystem(), 100.0f); // TODO: push down force

  ezVec3 vVelocityToApply = vClampedVelocity + vGroundVelocity;
  vVelocityToApply.z = m_fVelocityUp;

  if (m_GroundState == GroundState::OnGround)
  {
    // only step up or down, if we are firmly on the ground (not even sliding
    AdjustVelocityToStepUpOrDown(vVelocityToApply.z, m_GroundContact, tDiff);
  }

  {
    // ezHybridArray<ezCCContactPoint, 32> castContacts;
    // const ezVec3 vQueryPos = ownTransform.m_vPosition + ezVec3(0, 0, m_fMaxStepHeight + GetCurrentTotalHeight() * 0.5f);

    // JPH::CapsuleShape shape(GetCurrentHeightValue() * 0.5f, m_fCapsuleRadius);
    // CollectCastContacts(&castContacts, &shape, vQueryPos, vVelocityToApply * tDiff);

    // if (!castContacts.IsEmpty())
    //{
    //   vVelocityToApply.SetZero();
    // }

    m_pController->SetLinearVelocity(ezJoltConversionUtils::ToVec3(vVelocityToApply));
  }
  // ezJoltCharacterShapeComponent* pShape = nullptr;
  // GetWorld()->TryGetComponent(m_hCharacterShape, pShape);

  // ezGameObject* pHeadObject;
  // if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  //{
  //   ezVec3 pos = pHeadObject->GetLocalPosition();

  //  float fTimeDiff = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();
  //  fTimeDiff = ezMath::Max(fTimeDiff, 0.005f); // prevent stuff from breaking at high frame rates
  //  float fFactor = 1.0f - ezMath::Pow(0.001f, fTimeDiff);
  //  pos.z = ezMath::Lerp(pos.z, m_fHeadTargetHeight, fFactor);

  //  pHeadObject->SetLocalPosition(pos);
  //}

  ResetInputState();
  ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::White);
}

void ezJoltFpsCharacterControllerComponent::ResetInputState()
{
  m_InputJumpBit = 0;
  m_InputCrouchBit = 0;
  m_InputRunBit = 0;

  m_vInputDirection.SetZero();
}

void ezJoltFpsCharacterControllerComponent::ApplyPhysicsTransform()
{
  const ezVec3 vNewFootPos = ezJoltConversionUtils::ToVec3(m_pController->GetPosition());
  GetOwner()->SetGlobalPosition(vNewFootPos);

  if (m_RotateZ.GetRadian() != 0.0f)
  {
    ezQuat qRotZ;
    qRotZ.SetFromAxisAndAngle(ezVec3(0, 0, 1), m_RotateZ);
    GetOwner()->SetGlobalRotation(qRotZ * GetOwner()->GetGlobalRotation());
    m_RotateZ.SetRadian(0.0);
  }
}

void ezJoltFpsCharacterControllerComponent::ApplyCrouchState()
{
  if (m_InputCrouchBit == m_IsCrouchingBit)
    return;

  if (TryResize(m_InputCrouchBit ? m_fCrouchHeight : m_fCapsuleHeight))
    m_IsCrouchingBit = m_InputCrouchBit;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// void ezJoltFpsCharacterControllerComponent::SetFallbackWalkSurfaceFile(const char* szFile)
//{
//   if (!ezStringUtils::IsNullOrEmpty(szFile))
//   {
//     m_hFallbackWalkSurface = ezResourceManager::LoadResource<ezSurfaceResource>(szFile);
//   }
//
//   if (m_hFallbackWalkSurface.IsValid())
//     ezResourceManager::PreloadResource(m_hFallbackWalkSurface);
// }
//
// const char* ezJoltFpsCharacterControllerComponent::GetFallbackWalkSurfaceFile() const
//{
//   if (!m_hFallbackWalkSurface.IsValid())
//     return "";
//
//   return m_hFallbackWalkSurface.GetResourceID();
// }
//
// void ezJoltFpsCharacterControllerComponent::SetHeadObjectReference(const char* szReference)
//{
//   auto resolver = GetWorld()->GetGameObjectReferenceResolver();
//
//   if (!resolver.IsValid())
//     return;
//
//   m_hHeadObject = resolver(szReference, GetHandle(), "HeadObject");
// }

void ezJoltFpsCharacterControllerComponent::MoveCharacter(ezMsgMoveCharacterController& msg)
{
  const float fDistanceToMove = ezMath::Max(ezMath::Abs((float)(msg.m_fMoveForwards - msg.m_fMoveBackwards)), ezMath::Abs((float)(msg.m_fStrafeRight - msg.m_fStrafeLeft)));

  m_vInputDirection += ezVec3((float)(msg.m_fMoveForwards - msg.m_fMoveBackwards), (float)(msg.m_fStrafeRight - msg.m_fStrafeLeft), 0);
  m_vInputDirection.NormalizeIfNotZero(ezVec3::ZeroVector()).IgnoreResult();
  m_vInputDirection *= fDistanceToMove;

  m_RotateZ += m_RotateSpeed * (float)(msg.m_fRotateRight - msg.m_fRotateLeft);

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

void ezJoltFpsCharacterControllerComponent::TeleportCharacter(const ezVec3& vGlobalFootPos)
{
  if (m_pController == nullptr)
    return;

  m_pController->SetPosition(ezJoltConversionUtils::ToVec3(vGlobalFootPos));
}

bool ezJoltFpsCharacterControllerComponent::IsDestinationUnobstructed(const ezVec3& vGlobalFootPos, float fCharacterHeight)
{
  if (fCharacterHeight < 0.01f)
    fCharacterHeight = GetCurrentHeightValue();

  return !TestShapeOverlap(vGlobalFootPos, fCharacterHeight);
}

bool ezJoltFpsCharacterControllerComponent::IsTouchingGround()
{
  // TODO: better virtual interface (standing / sliding / etc)
  return m_GroundState != GroundState::InAir;
}

bool ezJoltFpsCharacterControllerComponent::IsCrouching()
{
  return m_IsCrouchingBit;
}

class ContactCollector : public JPH::CollideShapeCollector
{
public:
  ezDynamicArray<ezCCContactPoint>* m_pContacts = nullptr;
  const JPH::BodyLockInterface* m_pLockInterface = nullptr;

  virtual void AddHit(const JPH::CollideShapeResult& inResult) override
  {
    auto& contact = m_pContacts->ExpandAndGetRef();
    contact.m_vPosition = ezJoltConversionUtils::ToVec3(inResult.mContactPointOn2);
    contact.m_vContactNormal = ezJoltConversionUtils::ToVec3(-inResult.mPenetrationAxis.Normalized());
    contact.m_BodyID = inResult.mBodyID2;

    JPH::BodyLockRead lock(*m_pLockInterface, contact.m_BodyID);
    contact.m_vSurfaceNormal = ezJoltConversionUtils::ToVec3(lock.GetBody().GetWorldSpaceSurfaceNormal(inResult.mSubShapeID2, inResult.mContactPointOn2));
  }
};

class ContactCastCollector : public JPH::CastShapeCollector
{
public:
  ezDynamicArray<ezCCContactPoint>* m_pContacts = nullptr;
  const JPH::BodyLockInterface* m_pLockInterface = nullptr;

  virtual void AddHit(const JPH::ShapeCastResult& inResult) override
  {
    auto& contact = m_pContacts->ExpandAndGetRef();
    contact.m_vPosition = ezJoltConversionUtils::ToVec3(inResult.mContactPointOn2);
    contact.m_vContactNormal = ezJoltConversionUtils::ToVec3(-inResult.mPenetrationAxis.Normalized());
    contact.m_BodyID = inResult.mBodyID2;
    contact.m_fCastFraction = inResult.mFraction;

    JPH::BodyLockRead lock(*m_pLockInterface, contact.m_BodyID);
    contact.m_vSurfaceNormal = ezJoltConversionUtils::ToVec3(lock.GetBody().GetWorldSpaceSurfaceNormal(inResult.mSubShapeID2, inResult.mContactPointOn2));
  }
};

void ezJoltFpsCharacterControllerComponent::OnApplyRootMotion(ezMsgApplyRootMotion& msg)
{
  // m_vAbsoluteRootMotion = msg.m_vTranslation;
  // m_RotateZ += msg.m_RotationZ;
}

const ezJoltUserData* ezJoltFpsCharacterControllerComponent::GetUserData() const
{
  const ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  return &pModule->GetUserData(m_uiUserDataIndex);
}

bool ezJoltFpsCharacterControllerComponent::CanResize(float fNewHeightValue) const
{
  if (fNewHeightValue < m_fCurrentHeightValue)
    return true;

  return !TestShapeOverlap(GetOwner()->GetGlobalPosition(), fNewHeightValue);
}

void ezJoltFpsCharacterControllerComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, m_fCapsuleRadius), m_fCapsuleRadius), ezInvalidSpatialDataCategory);
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, m_fCapsuleHeight + m_fCapsuleRadius), m_fCapsuleRadius), ezInvalidSpatialDataCategory);
}

JPH::Ref<JPH::Shape> ezJoltFpsCharacterControllerComponent::CreateCharacterShape(float fCapsuleHeight) const
{
  EZ_ASSERT_DEBUG(fCapsuleHeight >= 0.0f, "Invalid character capsule height");

  JPH::CapsuleShapeSettings cap;
  cap.mRadius = m_fCapsuleRadius;
  cap.mHalfHeightOfCylinder = fCapsuleHeight * 0.5f;

  JPH::RotatedTranslatedShapeSettings up;
  up.mInnerShapePtr = cap.Create().Get();
  up.mPosition = JPH::Vec3(0, 0, fCapsuleHeight * 0.5f + m_fCapsuleRadius + m_fMaxStepHeight);
  up.mRotation = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), ezAngle::Degree(90).GetRadian());

  return up.Create().Get();
}

bool ezJoltFpsCharacterControllerComponent::TestShapeSweep(ezPhysicsCastResult& out_sweepResult, const ezVec3& vDirGlobal, float fDistance) const
{
  const ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  ezTransform t;
  t.SetIdentity();
  t.m_vPosition = GetOwner()->GetGlobalPosition();
  t.m_vPosition.z += GetCurrentTotalHeight() * 0.5f;
  t.m_vPosition.z += 0.01f;

  ezPhysicsQueryParameters params(m_uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);
  params.m_uiIgnoreObjectFilterID = m_uiObjectFilterID; // ignore the CC shape itself

  return pModule->SweepTestCapsule(out_sweepResult, m_fCapsuleRadius, GetCurrentHeightValue(), t, vDirGlobal, fDistance, params);
}

bool ezJoltFpsCharacterControllerComponent::TestShapeOverlap(const ezVec3& vGlobalFootPos, float fNewHeightValue) const
{
  const ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  // little offset upwards, to get out of the ground, otherwise the CC is stuck in it
  ezVec3 vCenterPos = vGlobalFootPos;
  vCenterPos.z += GetCurrentTotalHeight() * 0.5f;
  vCenterPos.z += (fNewHeightValue - GetCurrentHeightValue()) * 0.5f;
  vCenterPos.z += 0.02f;

  // TODO: ignore area below m_fMaxStepHeight

  ezPhysicsQueryParameters params(m_uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);
  params.m_uiIgnoreObjectFilterID = m_uiObjectFilterID; // ignore the CC shape itself

  return pModule->OverlapTestCapsule(m_fCapsuleRadius + 0.005f, fNewHeightValue, ezTransform(vCenterPos), params);
}

float ezJoltFpsCharacterControllerComponent::GetCurrentTotalHeight() const
{
  return GetCurrentHeightValue() + m_fCapsuleRadius + m_fCapsuleRadius;
}

bool ezJoltFpsCharacterControllerComponent::TryResize(float fNewHeightValue)
{
  if (!CanResize(fNewHeightValue))
    return false;

  if (m_pController->SetShape(CreateCharacterShape(fNewHeightValue).GetPtr(), FLT_MAX))
  {
    m_fCurrentHeightValue = fNewHeightValue;
    return true;
  }

  return false;
}

ezVec3 ezJoltFpsCharacterControllerComponent::ClampVelocityByContactPoints(ezVec3 vIntendedVelocity, const ezDynamicArray<ezCCContactPoint>& contacts) const
{
  ezVec3 vOrgDir = vIntendedVelocity;
  vOrgDir.z = 0;
  vOrgDir.NormalizeIfNotZero(ezVec3::ZeroVector()).IgnoreResult();

  bool bDebugDraw = m_DebugFlags.IsSet(ezJoltCCDebugFlags::VisContactPoints);

  for (ezUInt32 attempt = 0; attempt < 5; ++attempt)
  {
    bool bClampedByAny = false;

    for (const auto& contact : contacts)
    {
      if (contact.m_vSurfaceNormal.Dot(contact.m_vContactNormal) > ezMath::Cos(ezAngle::Degree(30)))
      {
        if (contact.m_vSurfaceNormal.Dot(ezVec3(0, 0, 1)) < ezMath::Cos(m_MaxClimbingSlope))
        {
          ezVec3 vContactDir2D = contact.m_vSurfaceNormal;
          vContactDir2D.z = 0;
          vContactDir2D.NormalizeIfNotZero(ezVec3::ZeroVector()).IgnoreResult();

          const float fProjection = vContactDir2D.Dot(vIntendedVelocity);

          if (fProjection < 0.0f)
          {
            bClampedByAny = true;

            vIntendedVelocity -= fProjection * vContactDir2D;
            vIntendedVelocity.z = 0;

            if (vOrgDir.Dot(vIntendedVelocity) < 0)
              vIntendedVelocity = -vIntendedVelocity;

            if (bDebugDraw)
            {
              ezTransform trans;
              trans.m_vPosition = contact.m_vPosition;
              trans.m_qRotation.SetShortestRotation(ezVec3::UnitXAxis(), vContactDir2D);
              trans.m_vScale.Set(1.0f);
              ezDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.1f, ezColor::ZeroColor(), ezColor::Purple, trans);
            }

            continue;
          }
        }
      }

      if (bDebugDraw)
      {
        ezColor color = ezColor::MediumPurple;

        if (contact.m_vSurfaceNormal.Dot(contact.m_vContactNormal) < ezMath::Cos(ezAngle::Degree(30)))
        {
          color = ezColor::DarkSlateGrey;
        }

        ezTransform trans;
        trans.m_vPosition = contact.m_vPosition;
        trans.m_qRotation.SetShortestRotation(ezVec3::UnitXAxis(), contact.m_vSurfaceNormal);
        trans.m_vScale.Set(1.0f);
        ezDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.1f, ezColor::ZeroColor(), color, trans);
      }
    }

    bDebugDraw = false;

    if (!bClampedByAny)
      return vIntendedVelocity;
  }

  return ezVec3::ZeroVector();
}

void ezJoltFpsCharacterControllerComponent::CollectContacts(ezDynamicArray<ezCCContactPoint>* out_pContacts, const JPH::Shape* pShape, const ezVec3& vQueryPosition, const ezVec3& vMoveDir) const
{
  const ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  const auto pJoltSystem = pModule->GetJoltSystem();

  ezJoltObjectLayerFilter objectFilter;
  objectFilter.m_uiCollisionLayer = m_uiCollisionLayer;

  ezJoltBroadPhaseLayerFilter broadphaseFilter(ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);
  ezJoltBodyFilter bodyFilter(m_uiObjectFilterID); // ignore the CC shape itself

  ContactCollector collector;
  collector.m_pLockInterface = &pJoltSystem->GetBodyLockInterfaceNoLock();
  collector.m_pContacts = out_pContacts;

  JPH::CollideShapeSettings settings;
  settings.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;
  settings.mMaxSeparationDistance = 0.05f;
  settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideOnlyWithActive;
  settings.mActiveEdgeMovementDirection = ezJoltConversionUtils::ToVec3(vMoveDir);

  // needed for capsules to be along the Z axis
  ezQuat qTiltX90;
  qTiltX90.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(90.0f));

  ezQuat qQueryRot = GetOwner()->GetGlobalRotation() * qTiltX90;
  qQueryRot.Normalize();

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(qQueryRot), ezJoltConversionUtils::ToVec3(vQueryPosition));
  pJoltSystem->GetNarrowPhaseQuery().CollideShape(pShape, JPH::Vec3(1, 1, 1), trans, settings, collector, broadphaseFilter, objectFilter, bodyFilter);
}

void ezJoltFpsCharacterControllerComponent::CollectCastContacts(ezDynamicArray<ezCCContactPoint>* out_pContacts, const JPH::Shape* pShape, const ezVec3& vQueryPosition, const ezVec3& vSweepDir) const
{
  const ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  const auto pJoltSystem = pModule->GetJoltSystem();

  ezJoltObjectLayerFilter objectFilter;
  objectFilter.m_uiCollisionLayer = m_uiCollisionLayer;

  ezJoltBroadPhaseLayerFilter broadphaseFilter(ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);
  ezJoltBodyFilter bodyFilter(m_uiObjectFilterID); // ignore the CC shape itself

  ContactCastCollector collector;
  collector.m_pLockInterface = &pJoltSystem->GetBodyLockInterfaceNoLock();
  collector.m_pContacts = out_pContacts;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(JPH::Quat::sFromTo(JPH::Vec3::sAxisY(), JPH::Vec3::sAxisZ()), ezJoltConversionUtils::ToVec3(vQueryPosition));

  JPH::ShapeCast castOpt(pShape, JPH::Vec3::sReplicate(1.0f), trans, ezJoltConversionUtils::ToVec3(vSweepDir));

  JPH::ShapeCastSettings settings;
  pJoltSystem->GetNarrowPhaseQuery().CastShape(castOpt, settings, collector, broadphaseFilter, objectFilter, bodyFilter);
}

void ezJoltFpsCharacterControllerComponent::DebugDraw_SideContactCylinder(const ezVec3& vCylinderBottom, const ezVec3& vCylinderTop, float fCylinderRadius) const
{
  if (!m_DebugFlags.IsSet(ezJoltCCDebugFlags::VisQueryShape))
    return;

  ezQuat qTiltY90;
  qTiltY90.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90.0f));

  ezTransform trans;
  trans.m_vScale.Set(1.0f);
  trans.m_vPosition = vCylinderBottom;
  trans.m_qRotation = GetOwner()->GetGlobalRotation() * qTiltY90;

  ezColor color = ezColor::YellowGreen;

  if (m_GroundState == GroundState::InAir)
    color = ezColor::LightSkyBlue;
  else if (m_GroundState == GroundState::Sliding)
    color = ezColor::DarkOrange;

  const float fCylinderHeight = vCylinderTop.z - vCylinderBottom.z;

  ezDebugRenderer::DrawCylinder(GetWorld(), fCylinderRadius, fCylinderRadius, fCylinderHeight, ezColor::ZeroColor(), color, trans);
}

void ezJoltFpsCharacterControllerComponent::AdjustVelocityToStepUpOrDown(float& fVelocityUp, const ezCCContactPoint& groundContact, float fTimeDiff) const
{
  if (fVelocityUp != 0.0f)
    return;

  const float fGroundConnectDist = ezMath::Clamp(groundContact.m_vPosition.z - GetOwner()->GetGlobalPosition().z, -m_fMaxStepHeight, +m_fMaxStepHeight);

  fVelocityUp += (fGroundConnectDist / fTimeDiff) * 0.25f;
}

ezVec3 ezJoltFpsCharacterControllerComponent::RestrictMovementByGeometry(const ezVec3& vIntendedMovement, float fCylinderHeight, float fCylinderRadius) const
{
  const ezTransform ownerTransform = GetOwner()->GetGlobalTransform();

  const ezVec3 vCylinderBottom = ownerTransform.m_vPosition + ezVec3(0, 0, m_fMaxStepHeight);
  const ezVec3 vCylinderTop = vCylinderBottom + ezVec3(0, 0, fCylinderHeight);

  DebugDraw_SideContactCylinder(vCylinderBottom, vCylinderTop, fCylinderRadius);

  if (vIntendedMovement.IsZero())
    return ezVec3::ZeroVector();

  const JPH::CylinderShape shape((vCylinderTop.z - vCylinderBottom.z) * 0.5f, fCylinderRadius);

  ezHybridArray<ezCCContactPoint, 32> contacts;
  CollectContacts(&contacts, &shape, ezMath::Lerp(vCylinderBottom, vCylinderTop, 0.5f), vIntendedMovement);

  return ClampVelocityByContactPoints(vIntendedMovement, contacts);
}

ezVec3 ezJoltFpsCharacterControllerComponent::GetGroundVelocityAndPushDown(const ezCCContactPoint& contact, JPH::PhysicsSystem* pJoltSystem, float fPushGroundForce)
{
  if (contact.m_BodyID.IsInvalid())
    return ezVec3::ZeroVector();

  JPH::BodyLockWrite bodyLock(pJoltSystem->GetBodyLockInterface(), contact.m_BodyID);

  if (!bodyLock.Succeeded())
    return ezVec3::ZeroVector();

  const JPH::Vec3 vGroundPos = ezJoltConversionUtils::ToVec3(contact.m_vPosition);

  if (fPushGroundForce > 0 && bodyLock.GetBody().IsDynamic())
  {
    const ezVec3 vPushDir = -contact.m_vSurfaceNormal * fPushGroundForce;

    bodyLock.GetBody().AddForce(ezJoltConversionUtils::ToVec3(vPushDir), vGroundPos);
    pJoltSystem->GetBodyInterfaceNoLock().ActivateBody(contact.m_BodyID);
  }

  ezVec3 vGroundVelocity = ezVec3::ZeroVector();

  if (bodyLock.GetBody().IsKinematic())
  {
    vGroundVelocity = ezJoltConversionUtils::ToVec3(bodyLock.GetBody().GetPointVelocity(vGroundPos));
    vGroundVelocity.z = 0;
  }

  return vGroundVelocity;
}

ezJoltFpsCharacterControllerComponent::GroundState ezJoltFpsCharacterControllerComponent::DetermineGroundState(ezCCContactPoint& out_Contact, const ezVec3& vFootPosition, float fAllowedStepUp, float fAllowedStepDown, float fCylinderRadius) const
{
  out_Contact.m_BodyID = JPH::BodyID(JPH::BodyID::cInvalidBodyID);

  const JPH::CylinderShape shape(0.2f, fCylinderRadius);

  const ezVec3 vStartSweep = vFootPosition + ezVec3(0, 0, 0.1f + fAllowedStepUp);
  const ezVec3 vEndSweep = vFootPosition + ezVec3(0, 0, 0.1f - fAllowedStepDown - 0.01f);

  ezHybridArray<ezCCContactPoint, 32> groundContacts;
  CollectCastContacts(&groundContacts, &shape, vStartSweep, vEndSweep - vStartSweep);

  if (groundContacts.IsEmpty())
    return GroundState::InAir;

  GroundState state = GroundState::Sliding;

  const float fMaxSlopeAngleCos = ezMath::Cos(m_MaxClimbingSlope);

  float fFlattestContactCos = -2.0f;    // overall flattest contact point
  float fClosestContactFraction = 2.0f; // closest contact point that is flat enough

  ezCCContactPoint сontactClosest;
  ezCCContactPoint сontactFlattest;

  for (const auto& contact : groundContacts)
  {
    const float fContactAngleCos = contact.m_vSurfaceNormal.Dot(ezVec3(0, 0, 1));

    if (fContactAngleCos > fMaxSlopeAngleCos) // is contact flat enough to stand on?
    {
      state = GroundState::OnGround;

      if (contact.m_fCastFraction < fClosestContactFraction) // contact closer than previous one?
      {
        fClosestContactFraction = contact.m_fCastFraction;
        сontactClosest = contact;
      }
    }

    if (fContactAngleCos > fFlattestContactCos) // is contact flatter than previous one?
    {
      fFlattestContactCos = fContactAngleCos;
      сontactFlattest = contact;
    }

    if (m_DebugFlags.IsSet(ezJoltCCDebugFlags::VisContactPoints))
    {
      const ezColor color = (fContactAngleCos > fMaxSlopeAngleCos) ? ezColor::RosyBrown : ezColor::OrangeRed;

      ezTransform trans;
      trans.m_vPosition = contact.m_vPosition;
      trans.m_qRotation.SetShortestRotation(ezVec3::UnitXAxis(), contact.m_vContactNormal);
      trans.m_vScale.Set(1.0f);
      ezDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.1f, ezColor::ZeroColor(), color, trans);
    }
  }

  if (state == GroundState::OnGround)
  {
    out_Contact = сontactClosest;
  }
  else
  {
    out_Contact = сontactFlattest;
  }

  return state;
}
