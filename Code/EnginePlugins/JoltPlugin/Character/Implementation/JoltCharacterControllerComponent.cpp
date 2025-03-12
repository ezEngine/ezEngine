#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/Stats.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezJoltCharacterDebugFlags, 1)
EZ_BITFLAGS_CONSTANTS(ezJoltCharacterDebugFlags::PrintState, ezJoltCharacterDebugFlags::VisShape, ezJoltCharacterDebugFlags::VisContacts,  ezJoltCharacterDebugFlags::VisCasts, ezJoltCharacterDebugFlags::VisGroundContact, ezJoltCharacterDebugFlags::VisFootCheck)
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezJoltCharacterControllerComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("PresenceCollisionLayer", m_uiPresenceCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_ACCESSOR_PROPERTY("Mass", GetMass, SetMass)->AddAttributes(new ezDefaultValueAttribute(70.0f), new ezClampValueAttribute(0.1f, 10000.0f)),
    EZ_ACCESSOR_PROPERTY("Strength", GetStrength, SetStrength)->AddAttributes(new ezDefaultValueAttribute(500.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("MaxClimbingSlope", GetMaxClimbingSlope, SetMaxClimbingSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(40))),
    EZ_BITFLAGS_MEMBER_PROPERTY("DebugFlags", ezJoltCharacterDebugFlags , m_DebugFlags),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Character"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezJoltCharacterControllerComponent::ezJoltCharacterControllerComponent() = default;
ezJoltCharacterControllerComponent::~ezJoltCharacterControllerComponent() = default;

void ezJoltCharacterControllerComponent::SetObjectToIgnore(ezUInt32 uiObjectFilterID)
{
  m_BodyFilter.m_uiObjectFilterIDToIgnore = uiObjectFilterID;
}

void ezJoltCharacterControllerComponent::ClearObjectToIgnore()
{
  m_BodyFilter.ClearFilter();
}

void ezJoltCharacterControllerComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_DebugFlags;

  s << m_uiCollisionLayer;
  s << m_uiPresenceCollisionLayer;
  s << m_fMass;
  s << m_fStrength;
  s << m_MaxClimbingSlope;
}

void ezJoltCharacterControllerComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_DebugFlags;

  s >> m_uiCollisionLayer;
  s >> m_uiPresenceCollisionLayer;
  s >> m_fMass;
  s >> m_fStrength;
  s >> m_MaxClimbingSlope;
}

void ezJoltCharacterControllerComponent::OnDeactivated()
{
  if (m_pCharacter)
  {
    if (ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>())
    {
      pModule->ActivateCharacterController(this, false);
    }

    m_pCharacter->Release();
    m_pCharacter = nullptr;
  }

  RemovePresenceBody();

  SUPER::OnDeactivated();
}

void ezJoltCharacterControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  JPH::CharacterVirtualSettings opt;
  opt.mUp = JPH::Vec3::sAxisZ();
  opt.mSupportingVolume = JPH::Plane(opt.mUp, -GetShapeRadius());
  opt.mShape = MakeNextCharacterShape();
  opt.mMaxSlopeAngle = m_MaxClimbingSlope.GetRadian();
  opt.mMass = m_fMass;
  opt.mMaxStrength = m_fStrength;

  const ezTransform ownTrans = GetOwner()->GetGlobalTransform();

  m_pCharacter = new JPH::CharacterVirtual(&opt, ezJoltConversionUtils::ToVec3(ownTrans.m_vPosition), ezJoltConversionUtils::ToQuat(ownTrans.m_qRotation), pModule->GetJoltSystem());
  m_pCharacter->AddRef();

  pModule->ActivateCharacterController(this, true);

  CreatePresenceBody();
}

void ezJoltCharacterControllerComponent::SetMaxClimbingSlope(ezAngle slope)
{
  m_MaxClimbingSlope = slope;

  if (m_pCharacter)
  {
    m_pCharacter->SetMaxSlopeAngle(m_MaxClimbingSlope.GetRadian());
  }
}

void ezJoltCharacterControllerComponent::SetMass(float fMass)
{
  m_fMass = fMass;

  if (m_pCharacter)
  {
    m_pCharacter->SetMass(m_fMass);
  }
}

void ezJoltCharacterControllerComponent::SetStrength(float fStrength)
{
  m_fStrength = fStrength;

  if (m_pCharacter)
  {
    m_pCharacter->SetMaxStrength(m_fStrength);
  }
}

ezResult ezJoltCharacterControllerComponent::TryChangeShape(JPH::Shape* pNewShape)
{
  ezJoltBroadPhaseLayerFilter broadphaseFilter(ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);
  ezJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  if (m_pCharacter->SetShape(pNewShape, 0.01f, broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator()))
  {
    RemovePresenceBody();
    CreatePresenceBody();

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezJoltCharacterControllerComponent::RawMoveWithVelocity(const ezVec3& vVelocity, float fMaxStairStepUp, float fMaxStepDown)
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  ezJoltBroadPhaseLayerFilter broadphaseFilter(ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);
  ezJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  m_pCharacter->SetLinearVelocity(ezJoltConversionUtils::ToVec3(vVelocity));

  // Settings for our update function
  JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
  updateSettings.mStickToFloorStepDown = JPH::Vec3(0, 0, -fMaxStepDown);
  updateSettings.mWalkStairsStepUp = fMaxStairStepUp > 0 ? JPH::Vec3(0, 0, fMaxStairStepUp) : JPH::Vec3::sZero();

  // Update the character position
  m_pCharacter->ExtendedUpdate(GetUpdateTimeDelta(), ezJoltConversionUtils::ToVec3(pModule->GetCharacterGravity()), updateSettings, broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator());

  GetOwner()->SetGlobalPosition(ezJoltConversionUtils::ToSimdVec3(m_pCharacter->GetPosition()));
}


void ezJoltCharacterControllerComponent::RawMoveIntoDirection(const ezVec3& vDirection)
{
  if (vDirection.IsZero())
    return;

  RawMoveWithVelocity(vDirection * GetInverseUpdateTimeDelta(), 0.0f, 0.0f);
}

void ezJoltCharacterControllerComponent::RawMoveToPosition(const ezVec3& vTargetPosition)
{
  RawMoveIntoDirection(vTargetPosition - GetOwner()->GetGlobalPosition());
}

void ezJoltCharacterControllerComponent::TeleportToPosition(const ezVec3& vGlobalFootPos)
{
  m_pCharacter->SetPosition(ezJoltConversionUtils::ToVec3(vGlobalFootPos));

  ezJoltBroadPhaseLayerFilter broadphaseFilter(ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);
  ezJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  m_pCharacter->RefreshContacts(broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator());
}

bool ezJoltCharacterControllerComponent::StickToGround(float fMaxDist)
{
  if (m_pCharacter->GetGroundState() != JPH::CharacterBase::EGroundState::InAir || m_pCharacter->GetGroundState() != JPH::CharacterBase::EGroundState::NotSupported)
    return false;

  ezJoltBroadPhaseLayerFilter broadphaseFilter(ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);
  ezJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  return m_pCharacter->StickToFloor(JPH::Vec3(0, 0, -fMaxDist), broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator());
}

void ezJoltCharacterControllerComponent::CollectCastContacts(ezDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const ezVec3& vQueryPosition, const ezQuat& qQueryRotation, const ezVec3& vSweepDir) const
{
  out_Contacts.Clear();

  class ContactCastCollector : public JPH::CastShapeCollector
  {
  public:
    ezDynamicArray<ContactPoint>* m_pContacts = nullptr;
    const JPH::BodyLockInterface* m_pLockInterface = nullptr;

    virtual void AddHit(const JPH::ShapeCastResult& result) override
    {
      auto& contact = m_pContacts->ExpandAndGetRef();
      contact.m_vPosition = ezJoltConversionUtils::ToVec3(result.mContactPointOn2);
      contact.m_vContactNormal = ezJoltConversionUtils::ToVec3(-result.mPenetrationAxis.Normalized());
      contact.m_BodyID = result.mBodyID2;
      contact.m_fCastFraction = result.mFraction;
      contact.m_SubShapeID = result.mSubShapeID2;

      JPH::BodyLockRead lock(*m_pLockInterface, contact.m_BodyID);
      contact.m_vSurfaceNormal = ezJoltConversionUtils::ToVec3(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, result.mContactPointOn2));
    }
  };

  const ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  const auto pJoltSystem = pModule->GetJoltSystem();

  ezJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);
  ezJoltBroadPhaseLayerFilter broadphaseFilter(ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);

  ContactCastCollector collector;
  collector.m_pLockInterface = &pJoltSystem->GetBodyLockInterfaceNoLock();
  collector.m_pContacts = &out_Contacts;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(qQueryRotation), ezJoltConversionUtils::ToVec3(vQueryPosition));

  JPH::RShapeCast castOpt(pShape, JPH::Vec3::sReplicate(1.0f), trans, ezJoltConversionUtils::ToVec3(vSweepDir));

  JPH::ShapeCastSettings settings;
  pJoltSystem->GetNarrowPhaseQuery().CastShape(castOpt, settings, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, m_BodyFilter);
}

void ezJoltCharacterControllerComponent::CollectContacts(ezDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const ezVec3& vQueryPosition, const ezQuat& qQueryRotation, float fCollisionTolerance) const
{
  out_Contacts.Clear();

  class ContactCollector : public JPH::CollideShapeCollector
  {
  public:
    ezDynamicArray<ContactPoint>* m_pContacts = nullptr;
    const JPH::BodyLockInterface* m_pLockInterface = nullptr;

    virtual void AddHit(const JPH::CollideShapeResult& result) override
    {
      auto& contact = m_pContacts->ExpandAndGetRef();
      contact.m_vPosition = ezJoltConversionUtils::ToVec3(result.mContactPointOn2);
      contact.m_vContactNormal = ezJoltConversionUtils::ToVec3(-result.mPenetrationAxis.Normalized());
      contact.m_BodyID = result.mBodyID2;
      contact.m_SubShapeID = result.mSubShapeID2;

      JPH::BodyLockRead lock(*m_pLockInterface, contact.m_BodyID);
      contact.m_vSurfaceNormal = ezJoltConversionUtils::ToVec3(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, result.mContactPointOn2));
    }
  };

  const ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  const auto pJoltSystem = pModule->GetJoltSystem();

  ezJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);
  ezJoltBroadPhaseLayerFilter broadphaseFilter(ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);

  ContactCollector collector;
  collector.m_pLockInterface = &pJoltSystem->GetBodyLockInterfaceNoLock();
  collector.m_pContacts = &out_Contacts;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(qQueryRotation), ezJoltConversionUtils::ToVec3(vQueryPosition));

  JPH::CollideShapeSettings settings;
  settings.mCollisionTolerance = fCollisionTolerance;
  settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;

  pJoltSystem->GetNarrowPhaseQuery().CollideShape(pShape, JPH::Vec3::sReplicate(1.0f), trans, settings, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, m_BodyFilter);
}

ezVec3 ezJoltCharacterControllerComponent::GetContactVelocityAndPushAway(const ContactPoint& contact, float fPushForce)
{
  if (contact.m_BodyID.IsInvalid())
    return ezVec3::MakeZero();

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  auto pJoltSystem = pModule->GetJoltSystem();

  JPH::BodyLockWrite bodyLock(pJoltSystem->GetBodyLockInterface(), contact.m_BodyID);

  if (!bodyLock.Succeeded())
    return ezVec3::MakeZero();

  const JPH::Vec3 vGroundPos = ezJoltConversionUtils::ToVec3(contact.m_vPosition);

  if (fPushForce > 0 && bodyLock.GetBody().IsDynamic())
  {
    const ezVec3 vPushDir = -contact.m_vSurfaceNormal * fPushForce;

    bodyLock.GetBody().AddForce(ezJoltConversionUtils::ToVec3(vPushDir), vGroundPos);
    pJoltSystem->GetBodyInterfaceNoLock().ActivateBody(contact.m_BodyID);
  }

  ezVec3 vGroundVelocity = ezVec3::MakeZero();

  if (bodyLock.GetBody().IsKinematic())
  {
    vGroundVelocity = ezJoltConversionUtils::ToVec3(bodyLock.GetBody().GetPointVelocity(vGroundPos));
    vGroundVelocity.z = 0;
  }

  return vGroundVelocity;
}

void ezJoltCharacterControllerComponent::SpawnContactInteraction(const ContactPoint& contact, const ezHashedString& sSurfaceInteraction, ezSurfaceResourceHandle hFallbackSurface, const ezVec3& vInteractionNormal)
{
  if (contact.m_BodyID.IsInvalid())
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  ezSurfaceResourceHandle hSurface = hFallbackSurface;

  JPH::BodyLockRead lock(pModule->GetJoltSystem()->GetBodyLockInterfaceNoLock(), contact.m_BodyID);
  if (lock.Succeeded())
  {
    auto pMat = static_cast<const ezJoltMaterial*>(lock.GetBody().GetShape()->GetMaterial(contact.m_SubShapeID));
    if (pMat && pMat->m_pSurface)
    {
      hSurface = static_cast<const ezJoltMaterial*>(pMat)->m_pSurface->GetResourceHandle();
    }
  }

  if (hSurface.IsValid())
  {
    ezResourceLock<ezSurfaceResource> pSurface(hSurface, ezResourceAcquireMode::AllowLoadingFallback);
    pSurface->InteractWithSurface(GetWorld(), ezGameObjectHandle(), contact.m_vPosition, contact.m_vSurfaceNormal, vInteractionNormal, sSurfaceInteraction, &GetOwner()->GetTeamID());
  }
}

void ezJoltCharacterControllerComponent::VisualizeContact(const ContactPoint& contact, const ezColor& color) const
{
  ezTransform trans;
  trans.m_vPosition = contact.m_vPosition;
  trans.m_qRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), contact.m_vContactNormal);
  trans.m_vScale.Set(1.0f);

  ezDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.1f, ezColor::MakeZero(), color, trans);
}

void ezJoltCharacterControllerComponent::VisualizeContacts(const ezDynamicArray<ContactPoint>& contacts, const ezColor& color) const
{
  for (const auto& ct : contacts)
  {
    VisualizeContact(ct, color);
  }
}

void ezJoltCharacterControllerComponent::Update(ezTime deltaTime)
{
  m_fUpdateTimeDelta = deltaTime.AsFloatInSeconds();
  m_fInverseUpdateTimeDelta = static_cast<float>(1.0 / deltaTime.GetSeconds());

  UpdateCharacter();

  MovePresenceBody(deltaTime);
}

void ezJoltCharacterControllerComponent::CreatePresenceBody()
{
  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  const ezSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  JPH::BodyCreationSettings bodyCfg;
  bodyCfg.SetShape(m_pCharacter->GetShape());

  ezJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  ezUInt32 m_uiObjectFilterID = pModule->CreateObjectFilterID();

  bodyCfg.mAllowSleeping = false;
  bodyCfg.mPosition = ezJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = ezJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType = JPH::EMotionType::Kinematic;
  bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiPresenceCollisionLayer, ezJoltBroadphaseLayer::Character);
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<ezUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  EZ_ASSERT_DEV(pBody != nullptr, "Jolt body creation failed. You need to increase the maximum number of bodies.");

  m_uiPresenceBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, true);
}

void ezJoltCharacterControllerComponent::RemovePresenceBody()
{
  if (m_uiPresenceBodyID == ezInvalidIndex)
    return;

  JPH::BodyID bodyId(m_uiPresenceBodyID);

  m_uiPresenceBodyID = ezInvalidIndex;

  if (bodyId.IsInvalid())
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  pBodies->RemoveBody(bodyId);
  pBodies->DestroyBody(bodyId);

  pModule->DeallocateUserData(m_uiUserDataIndex);
  // pModule->DeleteObjectFilterID(m_uiObjectFilterID);
}

void ezJoltCharacterControllerComponent::MovePresenceBody(ezTime deltaTime)
{
  if (m_uiPresenceBodyID == ezInvalidIndex)
    return;

  JPH::BodyID bodyId(m_uiPresenceBodyID);

  if (bodyId.IsInvalid())
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  if (!pBodies->IsAdded(bodyId))
    return;

  const ezSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  const float tDiff = deltaTime.AsFloatInSeconds();

  pBodies->MoveKinematic(bodyId, ezJoltConversionUtils::ToVec3(trans.m_Position), ezJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized(), tDiff);
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Character_Implementation_JoltCharacterControllerComponent);
