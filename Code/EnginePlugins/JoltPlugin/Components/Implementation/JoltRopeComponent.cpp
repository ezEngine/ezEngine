#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
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
EZ_BEGIN_COMPONENT_TYPE(ezJoltRopeComponent, 1, ezComponentMode::Dynamic)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("Anchor", DummyGetter, SetAnchorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_MEMBER_PROPERTY("AttachToOrigin", m_bAttachToOrigin)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("AttachToAnchor", m_bAttachToAnchor)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new ezDefaultValueAttribute(16), new ezClampValueAttribute(2, 64)),
      EZ_MEMBER_PROPERTY("Slack", m_fSlack)->AddAttributes(new ezDefaultValueAttribute(0.3f)),
      EZ_MEMBER_PROPERTY("Mass", m_fTotalMass)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 1000.0f)),
      EZ_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.01f, 0.5f)),
      EZ_MEMBER_PROPERTY("BendStiffness", m_fBendStiffness)->AddAttributes(new ezClampValueAttribute(0.0f,   ezVariant())),
      EZ_MEMBER_PROPERTY("MaxBend", m_MaxBend)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(30)), new ezClampValueAttribute(ezAngle::Degree(5), ezAngle::Degree(90))),
      EZ_MEMBER_PROPERTY("MaxTwist", m_MaxTwist)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(15)), new ezClampValueAttribute(ezAngle::Degree(0.01f), ezAngle::Degree(90))),
      EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
      EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
      EZ_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
      //EZ_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision), // works, but not exposed for now
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_MESSAGEHANDLERS
    {
      EZ_MESSAGE_HANDLER(ezMsgPhysicsAddForce, AddForceAtPos),
      EZ_MESSAGE_HANDLER(ezMsgPhysicsAddImpulse, AddImpulseAtPos),
    }
    EZ_END_MESSAGEHANDLERS;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Physics/Jolt/Animation"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltRopeComponent::ezJoltRopeComponent() = default;
ezJoltRopeComponent::~ezJoltRopeComponent() = default;

void ezJoltRopeComponent::SetSurfaceFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    ezResourceManager::PreloadResource(m_hSurface);
}

const char* ezJoltRopeComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

void ezJoltRopeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_uiCollisionLayer;
  s << m_uiPieces;
  s << m_fThickness;
  s << m_bAttachToOrigin;
  s << m_bAttachToAnchor;
  s << m_bSelfCollision;
  s << m_fGravityFactor;
  s << m_hSurface;
  s << m_MaxBend;
  s << m_MaxTwist;
  s << m_fBendStiffness;
  s << m_fTotalMass;
  s << m_fSlack;

  stream.WriteGameObjectHandle(m_hAnchor);
}

void ezJoltRopeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_uiPieces;
  s >> m_fThickness;
  s >> m_bAttachToOrigin;
  s >> m_bAttachToAnchor;
  s >> m_bSelfCollision;
  s >> m_fGravityFactor;
  s >> m_hSurface;
  s >> m_MaxBend;
  s >> m_MaxTwist;
  s >> m_fBendStiffness;
  s >> m_fTotalMass;
  s >> m_fSlack;

  m_hAnchor = stream.ReadGameObjectHandle();
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
  const ezTransform tRoot = GetOwner()->GetGlobalTransform();

  ezHybridArray<ezTransform, 65> pieces;
  float fPieceLength;
  if (CreateSegmentTransforms(pieces, fPieceLength).Failed())
    return;

  pieces.PopBack(); // don't need the last transform

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  m_uiObjectFilterID = pModule->CreateObjectFilterID();

  const ezJoltMaterial* pMaterial = GetJoltMaterial();

  ezJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  JPH::Ref<JPH::RagdollSettings> opt = new JPH::RagdollSettings();
  opt->mSkeleton = new JPH::Skeleton();
  opt->mSkeleton->GetJoints().resize(pieces.GetCount());
  opt->mParts.resize(pieces.GetCount());

  const float fMass = m_fTotalMass / pieces.GetCount();

  ezStringBuilder name;

  JPH::CapsuleShapeSettings capsule;
  capsule.mRadius = m_fThickness * 0.5f;
  capsule.mHalfHeightOfCylinder = fPieceLength * 0.5f;
  capsule.mMaterial = pMaterial;
  // TODO: capsule.mUserData = ... one for the entire rope ?

  JPH::RotatedTranslatedShapeSettings capsOffset;
  capsOffset.mInnerShapePtr = capsule.Create().Get();
  capsOffset.mPosition = JPH::Vec3(fPieceLength * 0.5f, 0, 0);
  capsOffset.mRotation = JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), ezAngle::Degree(-90).GetRadian());
  // TODO: capsOffset.mUserData = ... one for the entire rope ?

  ezVec3 vNextJointPos;

  for (ezUInt32 idx = 0; idx < pieces.GetCount(); ++idx)
  {
    // skeleton
    {
      auto& joint = opt->mSkeleton->GetJoint(idx);

      // set previous name as parent name
      joint.mParentName = name;

      name.Format("RopeLink_{}", idx);
      joint.mName = name;
      joint.mParentJointIndex = static_cast<ezInt32>(idx) - 1;
    }

    auto& part = opt->mParts[idx];
    part.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Rope);
    part.mGravityFactor = m_fGravityFactor;
    part.mMotionQuality = JPH::EMotionQuality::LinearCast; // todo: option
    part.mMotionType = JPH::EMotionType::Dynamic;
    part.mPosition = ezJoltConversionUtils::ToVec3(pieces[idx].m_vPosition);
    part.mRotation = ezJoltConversionUtils::ToQuat(pieces[idx].m_qRotation);
    part.mUserData = reinterpret_cast<ezUInt64>(pUserData);
    part.SetShape(capsOffset.Create().Get()); // shape is cached, only 1 is created
    part.mMassPropertiesOverride.mMass = fMass;
    part.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    part.mRestitution = pMaterial->m_fRestitution;
    part.mFriction = pMaterial->m_fFriction;
    part.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
    //bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // TODO: might need a custom group filter 

    if (idx > 0)
    {
      JPH::SwingTwistConstraintSettings* pConstraint = new JPH::SwingTwistConstraintSettings();
      pConstraint->mDrawConstraintSize = 0.1f;
      pConstraint->mPosition1 = ezJoltConversionUtils::ToVec3(vNextJointPos);
      pConstraint->mPosition2 = ezJoltConversionUtils::ToVec3(vNextJointPos);
      pConstraint->mNormalHalfConeAngle = m_MaxBend.GetRadian();
      pConstraint->mPlaneHalfConeAngle = m_MaxBend.GetRadian();
      pConstraint->mTwistAxis1 = ezJoltConversionUtils::ToVec3(pieces[idx - 1].m_qRotation * ezVec3(1, 0, 0));
      pConstraint->mTwistAxis2 = ezJoltConversionUtils::ToVec3(pieces[idx].m_qRotation * ezVec3(1, 0, 0));
      pConstraint->mPlaneAxis1 = ezJoltConversionUtils::ToVec3(pieces[idx - 1].m_qRotation * ezVec3(0, 1, 0));
      pConstraint->mPlaneAxis2 = ezJoltConversionUtils::ToVec3(pieces[idx].m_qRotation * ezVec3(0, 1, 0));
      pConstraint->mTwistMinAngle = -m_MaxTwist.GetRadian();
      pConstraint->mTwistMaxAngle = m_MaxTwist.GetRadian();
      pConstraint->mMaxFrictionTorque = m_fBendStiffness;
      part.mToParent = pConstraint;
    }

    vNextJointPos = pieces[idx].m_vPosition + pieces[idx].m_qRotation * ezVec3(fPieceLength * 0.5f, 0, 0);

    if ((m_bAttachToOrigin && idx == 0) || (m_bAttachToAnchor && idx + 1 == pieces.GetCount()))
    {
      // disable all collisions for the first and last rope segment
      // this prevents colliding with walls that the rope is attached to
      part.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Query);
    }
  }

  m_pRagdoll = opt->CreateRagdoll({}, reinterpret_cast<ezUInt64>(pUserData), pModule->GetJoltSystem());
  m_pRagdoll->AddRef();
  m_pRagdoll->AddToPhysicsSystem(JPH::EActivation::Activate);

  if (m_bAttachToOrigin)
  {
    m_pConstraintOrigin = CreateConstraint(GetOwner()->GetHandle(), pieces[0], m_pRagdoll->GetBodyID(0).GetIndexAndSequenceNumber());
  }

  if (m_bAttachToAnchor)
  {
    ezTransform localTransform;
    localTransform.SetIdentity();
    localTransform.m_vPosition.x = fPieceLength;

    ezTransform lastPiece;
    lastPiece.SetGlobalTransform(pieces.PeekBack(), localTransform);

    lastPiece.m_qRotation = -lastPiece.m_qRotation;

    m_pConstraintAnchor = CreateConstraint(m_hAnchor, lastPiece, m_pRagdoll->GetBodyIDs().back().GetIndexAndSequenceNumber());
  }
}

JPH::Constraint* ezJoltRopeComponent::CreateConstraint(const ezGameObjectHandle& hTarget, const ezTransform& location, ezUInt32 uiBodyID)
{
  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();


  JPH::BodyID bodyIDs[2] = {JPH::BodyID(JPH::BodyID::cInvalidBodyID), JPH::BodyID(uiBodyID)};

  ezJoltDynamicActorComponent* pActor = nullptr;

  ezGameObject* pObject = nullptr;
  if (GetWorld()->TryGetObject(hTarget, pObject))
  {
    if (!pObject->TryGetComponentOfBaseType(pActor))
    {
      pObject = pObject->GetParent();

      if (pObject)
      {
        pObject->TryGetComponentOfBaseType(pActor);
      }
    }
  }

  if (pActor)
  {
    pActor->EnsureSimulationStarted();
    bodyIDs[0] = JPH::BodyID(pActor->GetJoltBodyID());
  }

  // create the joint
  {
    JPH::BodyLockMultiWrite bodies(pModule->GetJoltSystem()->GetBodyLockInterface(), bodyIDs, 2);

    if (bodies.GetBody(1) == nullptr)
      return nullptr;

    JPH::Body* pAnchor = bodies.GetBody(0) != nullptr ? bodies.GetBody(0) : &JPH::Body::sFixedToWorld;

    // exact orientation of this constraint would need to be determined somehow
    // JPH::SwingTwistConstraintSettings constraint;
    // constraint.mDrawConstraintSize = 0.1f;
    // constraint.mPosition1 = ezJoltConversionUtils::ToVec3(location.m_vPosition);
    // constraint.mPosition2 = ezJoltConversionUtils::ToVec3(location.m_vPosition);
    // constraint.mNormalHalfConeAngle = m_MaxBend.GetRadian();
    // constraint.mPlaneHalfConeAngle = m_MaxBend.GetRadian();
    // constraint.mTwistAxis1 = ezJoltConversionUtils::ToVec3(location.m_qRotation * ezVec3(1, 0, 0));
    // constraint.mTwistAxis2 = ezJoltConversionUtils::ToVec3(location.m_qRotation * ezVec3(1, 0, 0));
    // constraint.mPlaneAxis1 = ezJoltConversionUtils::ToVec3(location.m_qRotation * ezVec3(0, 1, 0));
    // constraint.mPlaneAxis2 = ezJoltConversionUtils::ToVec3(location.m_qRotation * ezVec3(0, 1, 0));
    // constraint.mTwistMinAngle = -m_MaxTwist.GetRadian();
    // constraint.mTwistMaxAngle = m_MaxTwist.GetRadian();
    // constraint.mMaxFrictionTorque = m_fBendStiffness;

    JPH::PointConstraintSettings constraint;
    constraint.mDrawConstraintSize = 0.1f;
    constraint.mPoint1 = ezJoltConversionUtils::ToVec3(location.m_vPosition);
    constraint.mPoint2 = constraint.mPoint1;

    auto* pConstraint = constraint.Create(*pAnchor, *bodies.GetBody(1));
    pConstraint->AddRef();
    pModule->GetJoltSystem()->AddConstraint(pConstraint);

    return pConstraint;
  }
}

void ezJoltRopeComponent::UpdatePreview()
{
  ezVec3 vNewPreviewRefPos = GetOwner()->GetGlobalPosition();

  ezGameObject* pObj;
  if (GetWorld()->TryGetObject(m_hAnchor, pObj))
    vNewPreviewRefPos += pObj->GetGlobalPosition();

  // TODO: use a hash value instead
  vNewPreviewRefPos.x += m_fSlack;
  vNewPreviewRefPos.y += (float)m_uiPieces;

  if (vNewPreviewRefPos != m_vPreviewRefPos)
  {
    m_vPreviewRefPos = vNewPreviewRefPos;
    SendPreviewPose();
  }
}

ezResult ezJoltRopeComponent::CreateSegmentTransforms(ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength) const
{
  out_fPieceLength = 0.0f;

  if (m_uiPieces == 0)
    return EZ_FAILURE;

  const ezSimdVec4f vAnchorA = ezSimdConversion::ToVec3(GetOwner()->GetGlobalPosition());

  const ezGameObject* pAnchor = nullptr;
  if (!GetWorld()->TryGetObject(m_hAnchor, pAnchor))
    return EZ_FAILURE;

  const ezSimdVec4f vAnchorB = ezSimdConversion::ToVec3(pAnchor->GetGlobalPosition());

  const float fLength = (vAnchorB - vAnchorA).GetLength<3>();
  if (ezMath::IsZero(fLength, 0.001f))
    return EZ_FAILURE;

  // the rope simulation always introduces some sag,
  // (m_fSlack - 0.1f) puts the rope under additional tension to counteract the imprecise simulation
  // we could also drastically ramp up the simulation steps, but that costs way too much performance
  const float fIntendedRopeLength = fLength + fLength * (ezMath::Abs(m_fSlack) - 0.1f);

  ezRopeSimulator rope;
  rope.m_bFirstNodeIsFixed = true;
  rope.m_bLastNodeIsFixed = true;
  rope.m_fDampingFactor = 0.97f;
  rope.m_fSegmentLength = fIntendedRopeLength / m_uiPieces;
  rope.m_Nodes.SetCount(m_uiPieces + 1);
  rope.m_vAcceleration.Set(0, 0, ezMath::Sign(m_fSlack) * -1);

  for (ezUInt16 i = 0; i < m_uiPieces + 1; ++i)
  {
    rope.m_Nodes[i].m_vPosition = vAnchorA + (vAnchorB - vAnchorA) * ((float)i / (float)m_uiPieces);
    rope.m_Nodes[i].m_vPreviousPosition = rope.m_Nodes[i].m_vPosition;
  }

  rope.SimulateTillEquilibrium(0.001f, 200);

  transforms.SetCountUninitialized(m_uiPieces + 1);

  out_fPieceLength = 0.0f;

  for (ezUInt16 idx = 0; idx < m_uiPieces; ++idx)
  {
    const ezSimdVec4f p0 = rope.m_Nodes[idx].m_vPosition;
    const ezSimdVec4f p1 = rope.m_Nodes[idx + 1].m_vPosition;
    ezSimdVec4f dir = p1 - p0;

    const ezSimdFloat len = dir.GetLength<3>();
    out_fPieceLength += len;

    if (len <= 0.001f)
      dir = ezSimdVec4f(1, 0, 0, 0);
    else
      dir /= len;

    transforms[idx].m_vScale.Set(1);
    transforms[idx].m_vPosition = ezSimdConversion::ToVec3(p0);
    transforms[idx].m_qRotation.SetShortestRotation(ezVec3::UnitXAxis(), ezSimdConversion::ToVec3(dir));
  }

  out_fPieceLength /= m_uiPieces;

  {
    ezUInt32 idx = m_uiPieces;
    transforms[idx].m_vScale.Set(1);
    transforms[idx].m_vPosition = ezSimdConversion::ToVec3(rope.m_Nodes[idx].m_vPosition);
    transforms[idx].m_qRotation = transforms[idx - 1].m_qRotation;
  }

  return EZ_SUCCESS;
}

void ezJoltRopeComponent::DestroyPhysicsShapes()
{
  if (m_pRagdoll)
  {
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

    m_pRagdoll->RemoveFromPhysicsSystem();
    m_pRagdoll->Release();
    m_pRagdoll = nullptr;

    if (m_pConstraintOrigin)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintOrigin);
      m_pConstraintOrigin->Release();
      m_pConstraintOrigin = nullptr;
    }

    if (m_pConstraintAnchor)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor);
      m_pConstraintAnchor->Release();
      m_pConstraintAnchor = nullptr;
    }

    pModule->DeallocateUserData(m_uiUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }
}

// this is a hard-coded constant that seems to prevent the most severe issues well enough
//
// applying forces to an articulation is a problem,
// because articulations tend to end up with bad (NaN)
// data real quickly if those forces are too large
// linear and angular damping already helps a lot, but it's not sufficient
// the only reliable solution seems to be to prevent too large incoming forces
// since ropes have many links, things like explosions tend to apply the same force to each link
// thus multiplying the effect
// therefore a 'frame budget' is used to only apply a certain amount of force during a single frame
// this effectively ignores most forces that are applied to multiple links and just moves one or two links
// constexpr float g_fMaxForce = 1.5f;

void ezJoltRopeComponent::Update()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  // m_fMaxForcePerFrame = g_fMaxForce * 2.0f;

  if (m_pRagdoll == nullptr)
    return;

  // at runtime, allow to disengage the connection
  {
    if (!m_bAttachToOrigin && m_pConstraintOrigin)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintOrigin);
      m_pConstraintOrigin->Release();
      m_pConstraintOrigin = nullptr;
      m_pRagdoll->Activate();
    }

    if (!m_bAttachToAnchor && m_pConstraintAnchor)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor);
      m_pConstraintAnchor->Release();
      m_pConstraintAnchor = nullptr;
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

  // if one is inactive, all the linked bodies are inactive
  if (!pModule->GetJoltSystem()->GetBodyInterface().IsActive(m_pRagdoll->GetBodyID(0)))
    return;

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

      poses[i].SetLocalTransform(rootTransform, global);
    }
  }

  // last pose
  {
    const ezUInt32 uiLastIdx = static_cast<ezUInt32>(m_pRagdoll->GetBodyCount());

    ezTransform tLocal;
    tLocal.SetIdentity();
    tLocal.m_vPosition.x = (poses[uiLastIdx - 1].m_vPosition - poses[uiLastIdx - 2].m_vPosition).GetLength();

    poses.PeekBack().SetGlobalTransform(poses[uiLastIdx - 1], tLocal);
  }

  GetOwner()->SendMessage(poseMsg);
}

void ezJoltRopeComponent::SendPreviewPose()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  ezDynamicArray<ezTransform> pieces(ezFrameAllocator::GetCurrentAllocator());

  ezMsgRopePoseUpdated poseMsg;
  float fPieceLength;
  if (CreateSegmentTransforms(pieces, fPieceLength).Succeeded())
  {
    poseMsg.m_LinkTransforms = pieces;

    const ezTransform tOwner = GetOwner()->GetGlobalTransform();

    for (auto& n : pieces)
    {
      n.SetLocalTransform(tOwner, n);
    }
  }

  GetOwner()->PostMessage(poseMsg, ezTime::Zero(), ezObjectMsgQueueType::AfterInitialized);
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

void ezJoltRopeComponent::SetAnchorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor(resolver(szReference, GetHandle(), "Anchor"));
}

void ezJoltRopeComponent::SetAnchor(ezGameObjectHandle hActor)
{
  m_hAnchor = hActor;
}

void ezJoltRopeComponent::AddForceAtPos(ezMsgPhysicsAddForce& msg)
{
  // if (m_pArticulation == nullptr || m_fMaxForcePerFrame <= 0.0f)
  //   return;

  // EZ_PX_WRITE_LOCK(*m_pArticulation->getScene());

  // JoltArticulationLink* pLink[1] = {};

  // if (msg.m_pInternalPhysicsActor != nullptr)
  //   pLink[0] = reinterpret_cast<PxArticulationLink*>(msg.m_pInternalPhysicsActor);
  // else
  //   m_pArticulation->getLinks(pLink, 1);

  // ezVec3 vImp = msg.m_vForce;
  // const float fOrgImp = vImp.GetLength();

  // if (fOrgImp > g_fMaxForce)
  //{
  //   vImp.SetLength(g_fMaxForce).IgnoreResult();
  //   m_fMaxForcePerFrame -= g_fMaxForce;
  // }
  // else
  //{
  //   m_fMaxForcePerFrame -= fOrgImp;
  // }

  // JoltRigidBodyExt::addForceAtPos(*pLink[0], ezJoltConversionUtils::ToVec3(vImp), ezJoltConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eFORCE);
}

void ezJoltRopeComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg)
{
  // if (m_pArticulation == nullptr || m_fMaxForcePerFrame <= 0.0f)
  //   return;

  // EZ_PX_WRITE_LOCK(*m_pArticulation->getScene());

  // JoltArticulationLink* pLink[1] = {};

  // if (msg.m_pInternalPhysicsActor != nullptr)
  //   pLink[0] = reinterpret_cast<PxArticulationLink*>(msg.m_pInternalPhysicsActor);
  // else
  //   m_pArticulation->getLinks(pLink, 1);

  // ezVec3 vImp = msg.m_vImpulse;
  // const float fOrgImp = vImp.GetLength();

  // if (fOrgImp > g_fMaxForce)
  //{
  //   vImp.SetLength(g_fMaxForce).IgnoreResult();
  //   m_fMaxForcePerFrame -= g_fMaxForce;
  // }
  // else
  //{
  //   m_fMaxForcePerFrame -= fOrgImp;
  // }

  // JoltRigidBodyExt::addForceAtPos(*pLink[0], ezJoltConversionUtils::ToVec3(vImp), ezJoltConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eIMPULSE);
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
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void ezJoltRopeComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
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

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  if (pModule == nullptr)
    return;

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
    {
      it->Update();
    }
  }
}
