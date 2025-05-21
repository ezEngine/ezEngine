#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/PathComponent.h>
#include <GameEngine/Physics/RopeSimulator.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Components/JoltRopeComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <RendererCore/AnimationSystem/Declarations.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezJoltRopeAnchorConstraintMode, 1)
  EZ_ENUM_CONSTANTS(ezJoltRopeAnchorConstraintMode::None, ezJoltRopeAnchorConstraintMode::Point, ezJoltRopeAnchorConstraintMode::Fixed, ezJoltRopeAnchorConstraintMode::Cone)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezJoltRopeComponent, 4, ezComponentMode::Dynamic)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("Anchor1", DummyGetter, SetAnchor1Reference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_ACCESSOR_PROPERTY("Anchor2", DummyGetter, SetAnchor2Reference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_ENUM_ACCESSOR_PROPERTY("Anchor1Constraint", ezJoltRopeAnchorConstraintMode, GetAnchor1ConstraintMode, SetAnchor1ConstraintMode),
      EZ_ENUM_ACCESSOR_PROPERTY("Anchor2Constraint", ezJoltRopeAnchorConstraintMode, GetAnchor2ConstraintMode, SetAnchor2ConstraintMode),
      EZ_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new ezDefaultValueAttribute(16), new ezClampValueAttribute(2, 64)),
      EZ_MEMBER_PROPERTY("Slack", m_fSlack)->AddAttributes(new ezDefaultValueAttribute(0.3f)),
      EZ_MEMBER_PROPERTY("WeightCategory", m_uiWeightCategory)->AddAttributes(new ezDynamicEnumAttribute("PhysicsWeightCategory")),
      EZ_ACCESSOR_PROPERTY("WeightScale", GetWeight_Scale, SetWeight_Scale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
      EZ_ACCESSOR_PROPERTY("Mass", GetWeight_Mass, SetWeight_Mass)->AddAttributes(new ezSuffixAttribute(" kg"), new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(1.0f, 1000.0f)),
      EZ_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.01f, 0.5f)),
      EZ_MEMBER_PROPERTY("BendStiffness", m_fBendStiffness)->AddAttributes(new ezClampValueAttribute(0.0f,   ezVariant())),
      EZ_MEMBER_PROPERTY("MaxBend", m_MaxBend)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(30)), new ezClampValueAttribute(ezAngle::MakeFromDegree(5), ezAngle::MakeFromDegree(90))),
      EZ_MEMBER_PROPERTY("MaxTwist", m_MaxTwist)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(30)), new ezClampValueAttribute(ezAngle::MakeFromDegree(0.01f), ezAngle::MakeFromDegree(90))),
      EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
      EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
      EZ_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
      EZ_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
      EZ_MEMBER_PROPERTY("ContinuousCollisionDetection", m_bCCD),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_MESSAGEHANDLERS
    {
      EZ_MESSAGE_HANDLER(ezMsgPhysicsAddImpulse, AddImpulseAtPos),
      EZ_MESSAGE_HANDLER(ezJoltMsgDisconnectConstraints, OnJoltMsgDisconnectConstraints),
    }
    EZ_END_MESSAGEHANDLERS;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Physics/Jolt/Effects"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltRopeComponent::ezJoltRopeComponent() = default;
ezJoltRopeComponent::~ezJoltRopeComponent() = default;

void ezJoltRopeComponent::SetSurfaceFile(ezStringView sFile)
{
  if (!sFile.IsEmpty())
  {
    m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(sFile);
  }
  else
  {
    m_hSurface = {};
  }

  if (m_hSurface.IsValid())
    ezResourceManager::PreloadResource(m_hSurface);
}

ezStringView ezJoltRopeComponent::GetSurfaceFile() const
{
  return m_hSurface.GetResourceID();
}

void ezJoltRopeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_uiCollisionLayer;
  s << m_uiPieces;
  s << m_fThickness;
  s << m_Anchor1ConstraintMode;
  s << m_Anchor2ConstraintMode;
  s << m_bSelfCollision;
  s << m_fGravityFactor;
  s << m_hSurface;
  s << m_MaxBend;
  s << m_MaxTwist;
  s << m_fBendStiffness;
  s << m_uiWeightCategory;
  s << (float)m_fWeightScale;
  s << (float)m_fWeightMass;
  s << m_fSlack;
  s << m_bCCD;

  inout_stream.WriteGameObjectHandle(m_hAnchor1);
  inout_stream.WriteGameObjectHandle(m_hAnchor2);
}

void ezJoltRopeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  EZ_ASSERT_DEBUG(uiVersion >= 4, "Outdated version, please re-transform asset.");
  if (uiVersion < 4)
    return;

  auto& s = inout_stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_uiPieces;
  s >> m_fThickness;
  s >> m_Anchor1ConstraintMode;
  s >> m_Anchor2ConstraintMode;
  s >> m_bSelfCollision;
  s >> m_fGravityFactor;
  s >> m_hSurface;
  s >> m_MaxBend;
  s >> m_MaxTwist;
  s >> m_fBendStiffness;

  s >> m_uiWeightCategory;

  {
    float f;

    s >> f;
    m_fWeightScale = f;

    s >> f;
    m_fWeightMass = f;
  }

  s >> m_fSlack;
  s >> m_bCCD;

  m_hAnchor1 = inout_stream.ReadGameObjectHandle();
  m_hAnchor2 = inout_stream.ReadGameObjectHandle();
}

void ezJoltRopeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  CreateRope();
}

void ezJoltRopeComponent::OnActivated()
{
  UpdatePreview();
}

void ezJoltRopeComponent::OnDeactivated()
{
  DestroyPhysicsShapes();

  // tell the render components, that the rope is gone
  ezMsgRopePoseUpdated poseMsg;
  GetOwner()->SendMessage(poseMsg);

  SUPER::OnDeactivated();
}

const ezJoltMaterial* ezJoltRopeComponent::GetJoltMaterial()
{
  if (m_hSurface.IsValid())
  {
    ezResourceLock<ezSurfaceResource> pSurface(m_hSurface, ezResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      return static_cast<const ezJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  return ezJoltCore::GetDefaultMaterial();
}

void ezJoltRopeComponent::CreateRope()
{
  ezGameObjectHandle hAnchor1 = m_hAnchor1;
  ezGameObjectHandle hAnchor2 = m_hAnchor2;

  if (hAnchor1.IsInvalidated())
    hAnchor1 = GetOwner()->GetHandle();
  if (hAnchor2.IsInvalidated())
    hAnchor2 = GetOwner()->GetHandle();

  if (hAnchor1 == hAnchor2)
    return;

  ezHybridArray<ezTransform, 65> nodes;
  float fPieceLength;
  if (CreateSegmentTransforms(nodes, fPieceLength, hAnchor1, hAnchor2).Failed())
    return;

  const ezUInt32 numPieces = nodes.GetCount() - 1;

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  m_uiObjectFilterID = pModule->CreateObjectFilterID();

  const ezJoltMaterial* pMaterial = GetJoltMaterial();

  ezJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  JPH::Ref<JPH::RagdollSettings> opt = new JPH::RagdollSettings();
  opt->mSkeleton = new JPH::Skeleton();
  opt->mSkeleton->GetJoints().resize(numPieces);
  opt->mParts.resize(numPieces);

  const float fInitialMass = ezJoltCore::GetWeightCategoryConfig().GetMassForWeightCategory(m_uiWeightCategory, 5.0f, m_fWeightMass, m_fWeightScale);

  const float fPieceMass = fInitialMass / numPieces;

  ezStringBuilder name;

  JPH::CapsuleShapeSettings capsule;
  capsule.mRadius = m_fThickness * 0.5f;
  capsule.mHalfHeightOfCylinder = fPieceLength * 0.5f;
  capsule.mMaterial = pMaterial;
  capsule.mUserData = reinterpret_cast<ezUInt64>(pUserData);

  JPH::RotatedTranslatedShapeSettings capsOffset;
  capsOffset.mInnerShapePtr = capsule.Create().Get();
  capsOffset.mPosition = JPH::Vec3(fPieceLength * 0.5f, 0, 0);
  capsOffset.mRotation = JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), ezAngle::MakeFromDegree(-90).GetRadian());
  capsOffset.mUserData = reinterpret_cast<ezUInt64>(pUserData);

  for (ezUInt32 idx = 0; idx < numPieces; ++idx)
  {
    // skeleton
    {
      auto& joint = opt->mSkeleton->GetJoint(idx);

      // set previous name as parent name
      joint.mParentName = name;

      name.SetFormat("Link{}", idx);
      joint.mName = name;
      joint.mParentJointIndex = static_cast<ezInt32>(idx) - 1;
    }

    auto& part = opt->mParts[idx];
    part.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Rope);
    part.mGravityFactor = m_fGravityFactor;
    part.mMotionQuality = m_bCCD ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
    part.mMotionType = JPH::EMotionType::Dynamic;
    part.mPosition = ezJoltConversionUtils::ToVec3(nodes[idx].m_vPosition);
    part.mRotation = ezJoltConversionUtils::ToQuat(nodes[idx].m_qRotation);
    part.mUserData = reinterpret_cast<ezUInt64>(pUserData);
    part.SetShape(capsOffset.Create().Get()); // shape is cached, only 1 is created
    part.mMassPropertiesOverride.mMass = fPieceMass;
    part.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    part.mRestitution = pMaterial->m_fRestitution;
    part.mFriction = pMaterial->m_fFriction;
    part.mLinearDamping = 0.1f;
    part.mAngularDamping = 0.1f;
    part.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
    part.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilterIgnoreSame()); // this is used if m_bSelfCollision is off, otherwise it gets overridden below

    if (idx > 0)
    {
      JPH::SwingTwistConstraintSettings* pConstraint = new JPH::SwingTwistConstraintSettings();
      pConstraint->mDrawConstraintSize = 0.1f;
      pConstraint->mPosition1 = ezJoltConversionUtils::ToVec3(nodes[idx].m_vPosition);
      pConstraint->mPosition2 = pConstraint->mPosition1;
      pConstraint->mNormalHalfConeAngle = m_MaxBend.GetRadian();
      pConstraint->mPlaneHalfConeAngle = m_MaxBend.GetRadian();
      pConstraint->mTwistAxis1 = ezJoltConversionUtils::ToVec3(nodes[idx - 1].m_qRotation * ezVec3(1, 0, 0)).Normalized();
      pConstraint->mTwistAxis2 = ezJoltConversionUtils::ToVec3(nodes[idx].m_qRotation * ezVec3(1, 0, 0)).Normalized();
      pConstraint->mPlaneAxis1 = ezJoltConversionUtils::ToVec3(nodes[idx - 1].m_qRotation * ezVec3(0, 1, 0)).Normalized();
      pConstraint->mPlaneAxis2 = ezJoltConversionUtils::ToVec3(nodes[idx].m_qRotation * ezVec3(0, 1, 0)).Normalized();
      pConstraint->mTwistMinAngle = -m_MaxTwist.GetRadian();
      pConstraint->mTwistMaxAngle = m_MaxTwist.GetRadian();
      pConstraint->mMaxFrictionTorque = m_fBendStiffness;
      part.mToParent = pConstraint;
    }

    if ((m_Anchor1ConstraintMode != ezJoltRopeAnchorConstraintMode::None && idx == 0) ||
        (m_Anchor2ConstraintMode != ezJoltRopeAnchorConstraintMode::None && idx + 1 == numPieces))
    {
      // disable all collisions for the first and last rope segment
      // this prevents colliding with walls that the rope is attached to
      part.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Query);
    }
  }

  if (m_bSelfCollision)
  {
    // overrides the group filter above to one that allows collision with itself, except for directly joined bodies
    opt->DisableParentChildCollisions();
  }

  opt->Stabilize();

  m_pRagdoll = opt->CreateRagdoll(m_uiObjectFilterID, reinterpret_cast<ezUInt64>(pUserData), pModule->GetJoltSystem());
  m_pRagdoll->AddRef();
  m_pRagdoll->AddToPhysicsSystem(JPH::EActivation::Activate);

  if (m_Anchor1ConstraintMode != ezJoltRopeAnchorConstraintMode::None)
  {
    m_pConstraintAnchor1 = CreateConstraint(hAnchor1, nodes[0], m_pRagdoll->GetBodyID(0).GetIndexAndSequenceNumber(), m_Anchor1ConstraintMode, m_uiAnchor1BodyID);
  }

  if (m_Anchor2ConstraintMode != ezJoltRopeAnchorConstraintMode::None)
  {
    ezTransform end = nodes.PeekBack();
    end.m_qRotation = end.m_qRotation.GetInverse();
    m_pConstraintAnchor2 = CreateConstraint(hAnchor2, end, m_pRagdoll->GetBodyIDs().back().GetIndexAndSequenceNumber(), m_Anchor2ConstraintMode, m_uiAnchor2BodyID);
  }
}

JPH::Constraint* ezJoltRopeComponent::CreateConstraint(const ezGameObjectHandle& hTarget, const ezTransform& pieceLoc, ezUInt32 uiBodyID, ezJoltRopeAnchorConstraintMode::Enum mode, ezUInt32& out_uiConnectedToBodyID)
{
  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();


  JPH::BodyID bodyIDs[2] = {JPH::BodyID(JPH::BodyID::cInvalidBodyID), JPH::BodyID(uiBodyID)};

  ezGameObject* pTarget = nullptr;
  if (!GetWorld()->TryGetObject(hTarget, pTarget))
    return nullptr;

  // const auto targetLoc = pTarget->GetGlobalTransform();

  ezJoltDynamicActorComponent* pActor = nullptr;
  while (pTarget && !pTarget->TryGetComponentOfBaseType(pActor))
  {
    // search for a parent with a physics actor
    pTarget = pTarget->GetParent();
  }

  if (pActor)
  {
    pActor->EnsureSimulationStarted();
    out_uiConnectedToBodyID = pActor->GetJoltBodyID();
    bodyIDs[0] = JPH::BodyID(out_uiConnectedToBodyID);
  }


  // create the joint
  {
    JPH::BodyLockMultiWrite bodies(pModule->GetJoltSystem()->GetBodyLockInterface(), bodyIDs, 2);

    if (bodies.GetBody(1) == nullptr)
      return nullptr;

    JPH::Body* pAnchor = bodies.GetBody(0) != nullptr ? bodies.GetBody(0) : &JPH::Body::sFixedToWorld;

    const ezVec3 vTwistAxis1 = pieceLoc.m_qRotation * ezVec3(1, 0, 0);
    const ezVec3 vTwistAxis2 = pieceLoc.m_qRotation * ezVec3(1, 0, 0);

    const ezVec3 vOrthoAxis1 = pieceLoc.m_qRotation * ezVec3(0, 1, 0);
    const ezVec3 vOrthoAxis2 = pieceLoc.m_qRotation * ezVec3(0, 1, 0);

    JPH::Constraint* pConstraint = nullptr;

    if (mode == ezJoltRopeAnchorConstraintMode::Cone)
    {
      JPH::SwingTwistConstraintSettings constraint;
      constraint.mSpace = JPH::EConstraintSpace::WorldSpace;
      constraint.mDrawConstraintSize = 0.1f;
      constraint.mPosition1 = ezJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);
      constraint.mPosition2 = ezJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);
      constraint.mNormalHalfConeAngle = m_MaxBend.GetRadian();
      constraint.mPlaneHalfConeAngle = m_MaxBend.GetRadian();

      constraint.mTwistAxis1 = ezJoltConversionUtils::ToVec3(vTwistAxis1).Normalized();
      constraint.mTwistAxis2 = ezJoltConversionUtils::ToVec3(vTwistAxis2).Normalized();

      constraint.mPlaneAxis1 = ezJoltConversionUtils::ToVec3(vOrthoAxis1).Normalized();
      constraint.mPlaneAxis2 = ezJoltConversionUtils::ToVec3(vOrthoAxis2).Normalized();

      constraint.mTwistMinAngle = -m_MaxTwist.GetRadian();
      constraint.mTwistMaxAngle = m_MaxTwist.GetRadian();
      constraint.mMaxFrictionTorque = m_fBendStiffness;

      pConstraint = constraint.Create(*pAnchor, *bodies.GetBody(1));
    }
    else if (mode == ezJoltRopeAnchorConstraintMode::Point)
    {
      JPH::PointConstraintSettings constraint;
      constraint.mSpace = JPH::EConstraintSpace::WorldSpace;
      constraint.mDrawConstraintSize = 0.1f;
      constraint.mPoint1 = ezJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);
      constraint.mPoint2 = ezJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);

      pConstraint = constraint.Create(*pAnchor, *bodies.GetBody(1));
    }
    else if (mode == ezJoltRopeAnchorConstraintMode::Fixed)
    {
      JPH::FixedConstraintSettings constraint;
      constraint.mSpace = JPH::EConstraintSpace::WorldSpace;
      constraint.mDrawConstraintSize = 0.1f;
      constraint.mPoint1 = ezJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);
      constraint.mPoint2 = ezJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);
      constraint.mAxisX1 = ezJoltConversionUtils::ToVec3(vTwistAxis2).Normalized();
      constraint.mAxisY1 = ezJoltConversionUtils::ToVec3(vOrthoAxis2).Normalized();
      constraint.mAxisX2 = ezJoltConversionUtils::ToVec3(vTwistAxis2).Normalized();
      constraint.mAxisY2 = ezJoltConversionUtils::ToVec3(vOrthoAxis2).Normalized();

      pConstraint = constraint.Create(*pAnchor, *bodies.GetBody(1));
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }

    pConstraint->AddRef();
    pModule->GetJoltSystem()->AddConstraint(pConstraint);

    if (pActor)
    {
      pActor->AddConstraint(GetHandle());
    }

    return pConstraint;
  }
}

void ezJoltRopeComponent::UpdatePreview()
{
  ezGameObject* pAnchor1 = nullptr;
  ezGameObject* pAnchor2 = nullptr;
  if (!GetWorld()->TryGetObject(m_hAnchor1, pAnchor1))
    pAnchor1 = GetOwner();
  if (!GetWorld()->TryGetObject(m_hAnchor2, pAnchor2))
    pAnchor2 = GetOwner();

  if (pAnchor1 == pAnchor2)
    return;

  ezUInt32 uiHash = 0;
  ezQuat rot;

  ezVec3 pos = GetOwner()->GetGlobalPosition();
  uiHash = ezHashingUtils::xxHash32(&pos, sizeof(ezVec3), uiHash);

  pos = pAnchor1->GetGlobalPosition();
  uiHash = ezHashingUtils::xxHash32(&pos, sizeof(ezVec3), uiHash);
  rot = pAnchor1->GetGlobalRotation();
  uiHash = ezHashingUtils::xxHash32(&rot, sizeof(ezQuat), uiHash);

  pos = pAnchor2->GetGlobalPosition();
  uiHash = ezHashingUtils::xxHash32(&pos, sizeof(ezVec3), uiHash);
  rot = pAnchor2->GetGlobalRotation();
  uiHash = ezHashingUtils::xxHash32(&rot, sizeof(ezQuat), uiHash);

  uiHash = ezHashingUtils::xxHash32(&m_fSlack, sizeof(float), uiHash);
  uiHash = ezHashingUtils::xxHash32(&m_uiPieces, sizeof(ezUInt16), uiHash);
  uiHash = ezHashingUtils::xxHash32(&m_Anchor1ConstraintMode, sizeof(ezJoltRopeAnchorConstraintMode::StorageType), uiHash);
  uiHash = ezHashingUtils::xxHash32(&m_Anchor2ConstraintMode, sizeof(ezJoltRopeAnchorConstraintMode::StorageType), uiHash);

  if (uiHash != m_uiPreviewHash)
  {
    m_uiPreviewHash = uiHash;
    SendPreviewPose();
  }
}

ezResult ezJoltRopeComponent::CreateSegmentTransforms(ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength, ezGameObjectHandle hAnchor1, ezGameObjectHandle hAnchor2)
{
  out_fPieceLength = 0.0f;

  if (m_uiPieces == 0)
    return EZ_FAILURE;

  // ezPathComponent* pPath;
  // if (GetOwner()->TryGetComponentOfBaseType(pPath))
  //{
  //   // generally working, but the usability is still WIP

  //  pPath->EnsureLinearizedRepresentationIsUpToDate();

  //  const float fLength = pPath->GetLinearizedRepresentationLength();

  //  if (ezMath::IsZero(fLength, 0.001f))
  //    return EZ_FAILURE;

  //  const ezTransform ownTrans = GetOwner()->GetGlobalTransform();

  //  out_fPieceLength = fLength / m_uiPieces;

  //  transforms.SetCountUninitialized(m_uiPieces + 1);

  //  ezPathComponent::LinearSampler sampler;
  //  auto t0 = pPath->SampleLinearizedRepresentation(sampler);

  //  for (ezUInt16 p = 0; p < m_uiPieces; ++p)
  //  {
  //    float fAddDistance = out_fPieceLength;
  //    pPath->AdvanceLinearSamplerBy(sampler, fAddDistance);
  //    const auto t1 = pPath->SampleLinearizedRepresentation(sampler);

  //    transforms[p].m_vPosition = ownTrans * t0.m_vPosition;
  //    transforms[p].m_vScale.Set(1);
  //    transforms[p].m_qRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), ownTrans.m_qRotation * (t1.m_vPosition - t0.m_vPosition).GetNormalized());

  //    t0 = t1;
  //  }

  //  transforms.PeekBack().m_vPosition = ownTrans * t0.m_vPosition;
  //  transforms.PeekBack().m_vScale.Set(1);
  //  transforms.PeekBack().m_qRotation = transforms[m_uiPieces - 1].m_qRotation;

  //  return EZ_SUCCESS;
  //}
  // else
  {
    const ezGameObject* pAnchor1 = nullptr;
    const ezGameObject* pAnchor2 = nullptr;

    if (!GetWorld()->TryGetObject(hAnchor1, pAnchor1))
      return EZ_FAILURE;
    if (!GetWorld()->TryGetObject(hAnchor2, pAnchor2))
      return EZ_FAILURE;

    const ezVec3 vOrgAnchor1 = pAnchor1->GetGlobalPosition();
    const ezVec3 vOrgAnchor2 = pAnchor2->GetGlobalPosition();

    ezSimdVec4f vAnchor1 = ezSimdConversion::ToVec3(vOrgAnchor1);
    ezSimdVec4f vAnchor2 = ezSimdConversion::ToVec3(vOrgAnchor2);

    const float fLength = (vAnchor2 - vAnchor1).GetLength<3>();
    if (ezMath::IsZero(fLength, 0.001f))
      return EZ_FAILURE;

    // the rope simulation always introduces some sag,
    // (m_fSlack - 0.1f) puts the rope under additional tension to counteract the imprecise simulation
    // we could also drastically ramp up the simulation steps, but that costs way too much performance
    const float fIntendedRopeLength = fLength + fLength * (ezMath::Abs(m_fSlack) - 0.1f);

    const float fPieceLength = fIntendedRopeLength / m_uiPieces;

    ezUInt16 uiSimulatedPieces = m_uiPieces;

    bool bAnchor1Fixed = false;
    bool bAnchor2Fixed = false;

    if (m_Anchor1ConstraintMode == ezJoltRopeAnchorConstraintMode::Cone ||
        m_Anchor1ConstraintMode == ezJoltRopeAnchorConstraintMode::Fixed)
    {
      bAnchor1Fixed = true;
      vAnchor1 += ezSimdConversion::ToVec3(pAnchor1->GetGlobalDirForwards()) * fPieceLength;
      --uiSimulatedPieces;
    }

    if (m_Anchor2ConstraintMode == ezJoltRopeAnchorConstraintMode::Cone ||
        m_Anchor2ConstraintMode == ezJoltRopeAnchorConstraintMode::Fixed)
    {
      bAnchor2Fixed = true;
      vAnchor2 += ezSimdConversion::ToVec3(pAnchor2->GetGlobalDirForwards()) * fPieceLength;
      --uiSimulatedPieces;
    }

    ezRopeSimulator rope;
    rope.m_bFirstNodeIsFixed = true;
    rope.m_bLastNodeIsFixed = true;
    rope.m_fDampingFactor = 0.97f;
    rope.m_fSegmentLength = fPieceLength;
    rope.m_Nodes.SetCount(uiSimulatedPieces + 1);
    rope.m_vAcceleration.Set(0, 0, ezMath::Sign(m_fSlack) * -1);

    for (ezUInt16 i = 0; i < uiSimulatedPieces + 1; ++i)
    {
      rope.m_Nodes[i].m_vPosition = vAnchor1 + (vAnchor2 - vAnchor1) * ((float)i / (float)uiSimulatedPieces);
      rope.m_Nodes[i].m_vPreviousPosition = rope.m_Nodes[i].m_vPosition;
    }

    rope.SimulateTillEquilibrium(0.001f, 200);

    transforms.SetCountUninitialized(m_uiPieces + 1);

    ezUInt16 idx2 = 0;

    out_fPieceLength = 0.0f;

    if (bAnchor1Fixed)
    {
      out_fPieceLength += fPieceLength;

      transforms[idx2].m_vScale.Set(1);
      transforms[idx2].m_vPosition = vOrgAnchor1;
      transforms[idx2].m_qRotation = pAnchor1->GetGlobalRotation();

      ++idx2;
    }

    const float fRopeLen = rope.GetTotalLength();
    const float fRopePieceLen = fRopeLen / uiSimulatedPieces;

    ezSimdVec4f p0 = rope.m_Nodes[0].m_vPosition;

    for (ezUInt16 idx = 0; idx < uiSimulatedPieces; ++idx)
    {
      const ezSimdVec4f p1 = rope.GetPositionAtLength((idx + 1) * fRopePieceLen);
      ezSimdVec4f dir = p1 - p0;

      const ezSimdFloat len = dir.GetLength<3>();
      out_fPieceLength += len;

      if (len <= 0.001f)
        dir = ezSimdVec4f(1, 0, 0, 0);
      else
        dir /= len;

      transforms[idx2].m_vScale.Set(1);
      transforms[idx2].m_vPosition = ezSimdConversion::ToVec3(p0);
      transforms[idx2].m_qRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), ezSimdConversion::ToVec3(dir));

      ++idx2;
      p0 = p1;
    }

    {
      transforms[idx2].m_vScale.Set(1);
      transforms[idx2].m_vPosition = ezSimdConversion::ToVec3(rope.m_Nodes.PeekBack().m_vPosition);
      transforms[idx2].m_qRotation = transforms[idx2 - 1].m_qRotation;

      ++idx2;
    }

    if (bAnchor2Fixed)
    {
      out_fPieceLength += fPieceLength;

      transforms[idx2].m_vScale.Set(1);
      transforms[idx2].m_vPosition = vOrgAnchor2;
      transforms[idx2].m_qRotation = pAnchor2->GetGlobalRotation();

      ezVec3 dir = transforms[idx2].m_qRotation * ezVec3(1, 0, 0);
      transforms[idx2].m_qRotation = ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), -dir);

      // transforms[idx2].m_qRotation.Flip();
      transforms[idx2].m_qRotation.Normalize();
      transforms[idx2 - 1].m_qRotation = transforms[idx2].m_qRotation;
    }

    out_fPieceLength /= m_uiPieces;

    return EZ_SUCCESS;
  }
}

void ezJoltRopeComponent::DestroyPhysicsShapes()
{
  if (m_pRagdoll)
  {
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

    m_pRagdoll->RemoveFromPhysicsSystem();
    m_pRagdoll->Release();
    m_pRagdoll = nullptr;

    if (m_pConstraintAnchor1)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor1);
      m_pConstraintAnchor1->Release();
      m_pConstraintAnchor1 = nullptr;
      m_uiAnchor1BodyID = ezInvalidIndex;
    }

    if (m_pConstraintAnchor2)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor2);
      m_pConstraintAnchor2->Release();
      m_pConstraintAnchor2 = nullptr;
      m_uiAnchor2BodyID = ezInvalidIndex;
    }

    pModule->DeallocateUserData(m_uiUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }
}

// applying forces to a ragdoll is a problem,
// since ropes have many links, things like explosions tend to apply the same force to each link
// thus multiplying the effect
// the only reliable solution seems to be to prevent too large incoming forces
// therefore a 'frame budget' is used to only apply a certain amount of force during a single frame
// this effectively ignores most forces that are applied to multiple links and just moves one or two links
constexpr float g_fMaxForce = 1.5f;

void ezJoltRopeComponent::Update()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  m_fMaxForcePerFrame = g_fMaxForce * 2.0f;

  if (m_pRagdoll == nullptr)
    return;

  // at runtime, allow to disengage the connection
  {
    if (m_Anchor1ConstraintMode == ezJoltRopeAnchorConstraintMode::None && m_pConstraintAnchor1)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor1);
      m_pConstraintAnchor1->Release();
      m_pConstraintAnchor1 = nullptr;
      m_uiAnchor1BodyID = ezInvalidIndex;
      m_pRagdoll->Activate();
    }

    if (m_Anchor2ConstraintMode == ezJoltRopeAnchorConstraintMode::None && m_pConstraintAnchor2)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor2);
      m_pConstraintAnchor2->Release();
      m_pConstraintAnchor2 = nullptr;
      m_uiAnchor2BodyID = ezInvalidIndex;
      m_pRagdoll->Activate();
    }
  }

  // if (m_fWindInfluence > 0.0f)
  //{
  //   if (const ezWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<ezWindWorldModuleInterface>())
  //   {
  //     ezVec3 ropeDir = m_RopeSim.m_Nodes.PeekBack().m_vPosition - m_RopeSim.m_Nodes[0].m_vPosition;

  //    const ezVec3 vWind = pWind->GetWindAt(m_RopeSim.m_Nodes.PeekBack().m_vPosition) * m_fWindInfluence;

  //    ezVec3 windForce = vWind;
  //    windForce += pWind->ComputeWindFlutter(vWind, ropeDir, 10.0f, GetOwner()->GetStableRandomSeed());

  //    if (!windForce.IsZero())
  //    {
  //      // apply force to all articulation links
  //    }
  //  }
  //}

  ezHybridArray<ezTransform, 32> poses(ezFrameAllocator::GetCurrentAllocator());
  poses.SetCountUninitialized(static_cast<ezUInt32>(m_pRagdoll->GetBodyCount()) + 1);

  ezMsgRopePoseUpdated poseMsg;
  poseMsg.m_LinkTransforms = poses;

  JPH::Vec3 rootPos;
  JPH::Quat rootRot;
  m_pRagdoll->GetRootTransform(rootPos, rootRot);

  ezTransform rootTransform = GetOwner()->GetGlobalTransform();
  rootTransform.m_vPosition = ezJoltConversionUtils::ToVec3(rootPos);

  GetOwner()->SetGlobalPosition(rootTransform.m_vPosition);

  auto& lockInterface = pModule->GetJoltSystem()->GetBodyLockInterface();

  ezTransform global;
  global.m_vScale.Set(1);

  JPH::BodyLockMultiRead lock(lockInterface, m_pRagdoll->GetBodyIDs().data(), (int)m_pRagdoll->GetBodyCount());

  for (ezUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    if (auto pBody = lock.GetBody(i))
    {
      global.m_vPosition = ezJoltConversionUtils::ToVec3(pBody->GetPosition());
      global.m_qRotation = ezJoltConversionUtils::ToQuat(pBody->GetRotation());

      poses[i] = ezTransform::MakeLocalTransform(rootTransform, global);
    }
  }

  // last pose
  {
    const ezUInt32 uiLastIdx = static_cast<ezUInt32>(m_pRagdoll->GetBodyCount());

    ezTransform tLocal;
    tLocal.SetIdentity();
    tLocal.m_vPosition.x = (poses[uiLastIdx - 1].m_vPosition - poses[uiLastIdx - 2].m_vPosition).GetLength();

    poses.PeekBack() = ezTransform::MakeGlobalTransform(poses[uiLastIdx - 1], tLocal);
  }

  GetOwner()->SendMessage(poseMsg);
}

void ezJoltRopeComponent::SendPreviewPose()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  ezGameObjectHandle hAnchor1 = m_hAnchor1;
  ezGameObjectHandle hAnchor2 = m_hAnchor2;

  if (hAnchor1.IsInvalidated())
    hAnchor1 = GetOwner()->GetHandle();
  if (hAnchor2.IsInvalidated())
    hAnchor2 = GetOwner()->GetHandle();

  if (hAnchor1 == hAnchor2)
    return;

  ezDynamicArray<ezTransform> pieces(ezFrameAllocator::GetCurrentAllocator());

  ezMsgRopePoseUpdated poseMsg;
  float fPieceLength;
  if (CreateSegmentTransforms(pieces, fPieceLength, hAnchor1, hAnchor2).Succeeded())
  {
    poseMsg.m_LinkTransforms = pieces;

    const ezTransform tOwner = GetOwner()->GetGlobalTransform();

    for (auto& n : pieces)
    {
      n = ezTransform::MakeLocalTransform(tOwner, n);
    }
  }

  GetOwner()->PostMessage(poseMsg, ezTime::MakeZero(), ezObjectMsgQueueType::AfterInitialized);
}

void ezJoltRopeComponent::SetGravityFactor(float fGravity)
{
  if (m_fGravityFactor == fGravity)
    return;

  m_fGravityFactor = fGravity;

  if (!m_pRagdoll)
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  for (ezUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    pModule->GetJoltSystem()->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), m_fGravityFactor);
  }

  m_pRagdoll->Activate();
}

void ezJoltRopeComponent::SetAnchor1Reference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor1(resolver(szReference, GetHandle(), "Anchor1"));
}

void ezJoltRopeComponent::SetAnchor2Reference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor2(resolver(szReference, GetHandle(), "Anchor2"));
}

void ezJoltRopeComponent::SetAnchor1(ezGameObjectHandle hActor)
{
  m_hAnchor1 = hActor;
}

void ezJoltRopeComponent::SetAnchor2(ezGameObjectHandle hActor)
{
  m_hAnchor2 = hActor;
}

void ezJoltRopeComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg)
{
  if (m_pRagdoll == nullptr || m_fMaxForcePerFrame <= 0.0f)
    return;

  const float fImpulse = ezJoltCore::GetImpulseTypeConfig().GetImpulseForWeight(ref_msg.m_uiImpulseType, m_uiWeightCategory);

  ezUInt32 uiBodyID = ezInvalidIndex;

  if (ref_msg.m_pInternalPhysicsActor != nullptr)
    uiBodyID = (ezUInt32) reinterpret_cast<size_t>(ref_msg.m_pInternalPhysicsActor) & 0xFFFFFFFF;
  else
    uiBodyID = m_pRagdoll->GetBodyID(0).GetIndexAndSequenceNumber();

  ezVec3 vImp = ref_msg.m_vImpulse * fImpulse;
  const float fOrgImp = vImp.GetLength();

  if (fOrgImp > g_fMaxForce)
  {
    vImp.SetLength(g_fMaxForce).IgnoreResult();
    m_fMaxForcePerFrame -= g_fMaxForce;
  }
  else
  {
    m_fMaxForcePerFrame -= fOrgImp;
  }

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  pModule->AddImpulse(uiBodyID, vImp, ref_msg.m_vGlobalPosition);
}

void ezJoltRopeComponent::SetAnchor1ConstraintMode(ezEnum<ezJoltRopeAnchorConstraintMode> mode)
{
  if (m_Anchor1ConstraintMode == mode)
    return;

  m_Anchor1ConstraintMode = mode;

  if (mode == ezJoltRopeAnchorConstraintMode::None && m_pConstraintAnchor1)
  {
    m_pRagdoll->Activate();
  }
}

void ezJoltRopeComponent::SetAnchor2ConstraintMode(ezEnum<ezJoltRopeAnchorConstraintMode> mode)
{
  if (m_Anchor2ConstraintMode == mode)
    return;

  m_Anchor2ConstraintMode = mode;

  if (mode == ezJoltRopeAnchorConstraintMode::None && m_pConstraintAnchor2)
  {
    m_pRagdoll->Activate();
  }
}

void ezJoltRopeComponent::OnJoltMsgDisconnectConstraints(ezJoltMsgDisconnectConstraints& ref_msg)
{
  if (m_pConstraintAnchor1 && ref_msg.m_uiJoltBodyID == m_uiAnchor1BodyID)
  {
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor1);
    m_pConstraintAnchor1->Release();
    m_pConstraintAnchor1 = nullptr;
    m_uiAnchor1BodyID = ezInvalidIndex;
  }

  if (m_pConstraintAnchor2 && ref_msg.m_uiJoltBodyID == m_uiAnchor2BodyID)
  {
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

    pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor2);
    m_pConstraintAnchor2->Release();
    m_pConstraintAnchor2 = nullptr;
    m_uiAnchor2BodyID = ezInvalidIndex;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezJoltRopeComponentManager::ezJoltRopeComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

ezJoltRopeComponentManager::~ezJoltRopeComponentManager() = default;

void ezJoltRopeComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltRopeComponentManager::Update, this);
    desc.m_Phase = ezWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void ezJoltRopeComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  EZ_PROFILE_SCOPE("UpdateRopes");

  if (!GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActiveAndInitialized())
      {
        it->UpdatePreview();
      }
    }

    return;
  }

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  for (auto itActor : pModule->GetActiveRopes())
  {
    itActor.Key()->Update();
  }
}

EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltRopeComponent);
