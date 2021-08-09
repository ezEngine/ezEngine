#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxRagdollComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/Utilities/PxUserData.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <foundation/Px.h>

using namespace physx;

/*
* angular momentum preservation
* joint limit config
* joint limit frame positioning
* materials
* force application
* mass distribution
* communication with anim controller
* dummy poses for bones without shape
* drive to pose
*/

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPxRagdollStart, 1)
  EZ_ENUM_CONSTANTS(ezPxRagdollStart::BindPose, ezPxRagdollStart::WaitForPose, ezPxRagdollStart::WaitForPoseAndVelocity, ezPxRagdollStart::Wait)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezPxRagdollComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Start", ezPxRagdollStart, m_Start),
    EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_ACCESSOR_PROPERTY("DisableGravity", GetDisableGravity, SetDisableGravity),
    EZ_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    EZ_MESSAGE_HANDLER(ezMsgPhysicsAddForce, AddForceAtPos),
    EZ_MESSAGE_HANDLER(ezMsgPhysicsAddImpulse, AddImpulseAtPos),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxRagdollComponent::ezPxRagdollComponent() = default;
ezPxRagdollComponent::~ezPxRagdollComponent() = default;

void ezPxRagdollComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_Start;
  s << m_bDisableGravity;
  s << m_uiCollisionLayer;
  s << m_bSelfCollision;
  s << m_hSurface;
}

void ezPxRagdollComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_Start;
  s >> m_bDisableGravity;
  s >> m_uiCollisionLayer;
  s >> m_bSelfCollision;
  s >> m_hSurface;
}

void ezPxRagdollComponent::SetSurfaceFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    ezResourceManager::PreloadResource(m_hSurface);
}

const char* ezPxRagdollComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

PxMaterial* ezPxRagdollComponent::GetPxMaterial()
{
  if (m_hSurface.IsValid())
  {
    ezResourceLock<ezSurfaceResource> pSurface(m_hSurface, ezResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterial != nullptr)
    {
      return static_cast<PxMaterial*>(pSurface->m_pPhysicsMaterial);
    }
  }

  return ezPhysX::GetSingleton()->GetDefaultMaterial();
}

PxFilterData ezPxRagdollComponent::CreateFilterData()
{
  return ezPhysX::CreateFilterData(m_uiCollisionLayer, m_uiShapeID);
}

void ezPxRagdollComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_Start == ezPxRagdollStart::BindPose)
  {
    CreateShapesFromBindPose();
  }
}

void ezPxRagdollComponent::OnDeactivated()
{
  DestroyPhysicsShapes();

  SUPER::OnDeactivated();
}

void ezPxRagdollComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& poseMsg)
{
  if (!IsActiveAndSimulating())
    return;

  if (m_bShapesCreated)
    return;

  if (m_Start == ezPxRagdollStart::Wait || m_Start == ezPxRagdollStart::BindPose)
    return;

  ezMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  m_hSkeleton = msg.m_hSkeleton;

  if (m_bHasFirstState == false && m_Start == ezPxRagdollStart::WaitForPoseAndVelocity)
  {
    m_bHasFirstState = true;

    m_vLastPos.SetCountUninitialized(poseMsg.m_ModelTransforms.GetCount());

    for (ezUInt32 i = 0; i < poseMsg.m_ModelTransforms.GetCount(); ++i)
    {
      m_vLastPos[i] = poseMsg.m_ModelTransforms[i].GetTranslationVector();
    }

    return;
  }

  CreatePhysicsShapes(msg.m_hSkeleton, poseMsg);
}

void ezPxRagdollComponent::SetDisableGravity(bool b)
{
  if (m_bDisableGravity == b)
    return;

  m_bDisableGravity = b;

  if (m_pArticulation)
  {
    EZ_PX_WRITE_LOCK(*(m_pArticulation->getScene()));

    PxArticulationLink* pLinks[12] = {};
    const ezUInt32 uiNumLinks = m_pArticulation->getNbLinks();

    for (ezUInt32 uiFirstLink = 0; uiFirstLink < uiNumLinks; uiFirstLink += EZ_ARRAY_SIZE(pLinks))
    {
      const ezUInt32 numGotten = m_pArticulation->getLinks(pLinks, EZ_ARRAY_SIZE(pLinks), uiFirstLink);

      for (ezUInt32 i = 0; i < numGotten; ++i)
      {
        pLinks[i]->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
      }
    }

    m_pArticulation->wakeUp();
  }
}

static void ComputeBoneRotation(const ezMat4& fullTransform, ezQuat& rotationOnly)
{
  // the bone might contain (non-uniform) scaling and mirroring, which the quaternion can't represent
  // so reconstruct a representable rotation matrix
  {
    const ezVec3 x = fullTransform.TransformDirection(ezVec3(1, 0, 0)).GetNormalized();
    const ezVec3 y = fullTransform.TransformDirection(ezVec3(0, 1, 0)).GetNormalized();
    const ezVec3 z = x.CrossRH(y);

    ezMat3 m;
    m.SetColumn(0, x);
    m.SetColumn(1, y);
    m.SetColumn(2, z);

    rotationOnly.SetFromMat3(m);
  }
}

void ezPxRagdollComponent::CreatePhysicsShapes(const ezSkeletonResourceHandle& hSkeleton, ezMsgAnimationPoseUpdated& poseMsg)
{
  m_bShapesCreated = true;

  ezResourceLock<ezSkeletonResource> pSkeleton(hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  m_uiShapeID = pModule->CreateShapeId();

  ezPhysX* pEzPhysX = ezPhysX::GetSingleton();
  PxPhysics* pPxPhysX = pEzPhysX->GetPhysXAPI();

  const ezVec3 velocity = GetOwner()->GetVelocity();

  const PxMaterial* pPxMaterial = GetPxMaterial();
  const PxFilterData filter = CreateFilterData();

  ezPxUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  struct LinkData
  {
    PxArticulationLink* m_pLink = nullptr;
    ezTransform m_GlobalTransform;
  };

  ezMap<ezUInt16, LinkData> links;

  ezQuat qRotCapsule;
  qRotCapsule.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));

  m_pArticulation = pPxPhysX->createArticulation();
  m_pArticulation->userData = pUserData;

  //pArt->setSolverIterationCounts(64, 32);
  //pArt->setMaxProjectionIterations(8);

  const ezTransform tRoot = GetOwner()->GetGlobalTransform();

  // dummy root link
  links.FindOrAdd(ezInvalidJointIndex);

  m_ArticulationLinks.SetCount(poseMsg.m_ModelTransforms.GetCount());

  float invTime = 1.0f / GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  ezTransform rt = desc.m_RootTransform;

  ezMat4 scale;
  scale.SetScalingMatrix(desc.m_RootTransform.m_vScale);

  ezVec3 diff;
  diff.SetZero();

  for (ezUInt32 idx = 0; idx < desc.m_Geometry.GetCount(); ++idx)
  {
    const auto& geo = desc.m_Geometry[idx];

    if (geo.m_Type == ezSkeletonJointGeometryType::None)
      continue;

    ezUInt16 uiParentJoint = desc.m_Skeleton.GetJointByIndex(geo.m_uiAttachedToJoint).GetParentIndex();

    while (!links.Contains(uiParentJoint))
    {
      uiParentJoint = desc.m_Skeleton.GetJointByIndex(uiParentJoint).GetParentIndex();
    }

    const auto& parentLink = links[uiParentJoint];
    auto& thisLink = links[geo.m_uiAttachedToJoint];

    ezVec3 vOffsetPos = geo.m_Transform.m_qRotation * geo.m_Transform.m_vPosition;
    ezQuat qOffsetRot = geo.m_Transform.m_qRotation;

    if (thisLink.m_pLink == nullptr)
    {
      ezMat4 mFullTransform;
      ezQuat qFullRot;
      poseMsg.ComputeFullBoneTransform(geo.m_uiAttachedToJoint, mFullTransform, qFullRot);

      ezTransform linkTransform;
      linkTransform.SetIdentity();

      linkTransform.m_vPosition = mFullTransform.GetTranslationVector();
      linkTransform.m_qRotation = qFullRot;

      thisLink.m_GlobalTransform.SetGlobalTransform(tRoot, linkTransform);
      thisLink.m_pLink = m_pArticulation->createLink(parentLink.m_pLink, ezPxConversionUtils::ToTransform(thisLink.m_GlobalTransform));

      {
        if (m_bHasFirstState)
        {
          diff = poseMsg.m_ModelTransforms[geo.m_uiAttachedToJoint].GetTranslationVector() - m_vLastPos[geo.m_uiAttachedToJoint];
          diff *= invTime;
          //ezLog::Dev("diff: {}, {}, {}", diff.x, diff.y, diff.z);
        }

        thisLink.m_pLink->setLinearVelocity(ezPxConversionUtils::ToVec3(velocity + diff));
      }

      EZ_ASSERT_DEV(thisLink.m_pLink != nullptr, "Ragdoll shape creation failed. Too many bones? (max 64)");

      m_ArticulationLinks[geo.m_uiAttachedToJoint].m_pLink = thisLink.m_pLink;

      thisLink.m_pLink->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
      thisLink.m_pLink->userData = pUserData;

      if (parentLink.m_pLink != nullptr)
      {
        ezTransform parentFrameJoint;
        parentFrameJoint.SetLocalTransform(parentLink.m_GlobalTransform, thisLink.m_GlobalTransform);

        if (false) // relative to bind pose
        {
          const ezMat4 bp = scale * pSkeleton->GetDescriptor().m_Skeleton.GetJointByIndex(geo.m_uiAttachedToJoint).GetBindPoseLocalTransform().GetAsMat4();

          parentFrameJoint.SetIdentity();
          ComputeBoneRotation(bp, parentFrameJoint.m_qRotation);
          parentFrameJoint.m_vPosition = bp.GetTranslationVector();
        }

        PxArticulationJoint* inb = reinterpret_cast<PxArticulationJoint*>(thisLink.m_pLink->getInboundJoint());

        inb->setParentPose(ezPxConversionUtils::ToTransform(parentFrameJoint));
        inb->setSwingLimitEnabled(true);
        inb->setTwistLimitEnabled(true);
        inb->setSwingLimit(ezAngle::Degree(35).GetRadian(), ezAngle::Degree(35).GetRadian());
        //inb->setSwingLimit(ezAngle::Degree(5).GetRadian(), ezAngle::Degree(5).GetRadian());
        inb->setTwistLimit(ezAngle::Degree(-20).GetRadian(), ezAngle::Degree(20).GetRadian());
        //inb->setTwistLimit(ezAngle::Degree(-5).GetRadian(), ezAngle::Degree(5).GetRadian());
        inb->setTangentialDamping(25.0f);
        inb->setDamping(25.0f);

        //inb->setDriveType(PxArticulationJointDriveType::eTARGET);
        //inb->setTargetOrientation(ezPxConversionUtils::ToQuat(ezQuat::IdentityQuaternion()));
        //inb->setStiffness(1000.0f);
        //inb->setDamping(50.0f);
        inb->setInternalCompliance(0.7f);
        inb->setExternalCompliance(1.0f);
      }
      else
      {
        m_pRootLink = thisLink.m_pLink;
      }
    }

    const float fScale = 1.0f;

    PxShape* pShape = nullptr;

    if (geo.m_Type == ezSkeletonJointGeometryType::Sphere)
    {
      PxSphereGeometry shape(fScale * geo.m_Transform.m_vScale.z);
      pShape = PxRigidActorExt::createExclusiveShape(*thisLink.m_pLink, shape, *pPxMaterial);
    }
    else if (geo.m_Type == ezSkeletonJointGeometryType::Box)
    {
      ezVec3 ext;
      ext.x = geo.m_Transform.m_vScale.y * 0.5f;
      ext.y = geo.m_Transform.m_vScale.x * 0.5f;
      ext.z = geo.m_Transform.m_vScale.z * 0.5f;
      ext *= fScale;

      PxBoxGeometry shape(ext.x, ext.y, ext.z);
      pShape = PxRigidActorExt::createExclusiveShape(*thisLink.m_pLink, shape, *pPxMaterial);

      vOffsetPos += geo.m_Transform.m_qRotation * ezVec3(0, geo.m_Transform.m_vScale.x * 0.5f, 0);
    }
    else if (geo.m_Type == ezSkeletonJointGeometryType::Capsule)
    {
      PxCapsuleGeometry shape(fScale * geo.m_Transform.m_vScale.z, fScale * geo.m_Transform.m_vScale.x * 0.5f);
      pShape = PxRigidActorExt::createExclusiveShape(*thisLink.m_pLink, shape, *pPxMaterial);

      vOffsetPos += geo.m_Transform.m_qRotation * ezVec3(0, geo.m_Transform.m_vScale.x * 0.5f, 0);
      qOffsetRot = qOffsetRot * qRotCapsule;
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }

    //pShape->SetSurfaceFile(geo.m_sSurface);

    vOffsetPos = vOffsetPos.CompMul(poseMsg.m_pRootTransform->m_vScale);

    pShape->setLocalPose(ezPxConversionUtils::ToTransform(ezTransform(vOffsetPos, qOffsetRot)));
    pShape->setSimulationFilterData(filter);
    pShape->setQueryFilterData(filter);
    pShape->userData = pUserData;

    float fMass = 5.0f;

    if (parentLink.m_pLink)
    {
      fMass = 0.9f * parentLink.m_pLink->getMass();
    }

    PxRigidBodyExt::setMassAndUpdateInertia(*thisLink.m_pLink, fMass);
  }

  EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

  m_pAggregate = pPxPhysX->createAggregate(64, m_bSelfCollision);
  m_pAggregate->addArticulation(*m_pArticulation);
  pModule->GetPxScene()->addAggregate(*m_pAggregate);

  poseMsg.m_bContinueAnimating = false;

  {
    PxArticulationLink* pLink[1] = {};
    m_pArticulation->getLinks(pLink, 1);

    for (const auto& imp : m_Impulses)
    {
      PxRigidBodyExt::addForceAtPos(*pLink[0], ezPxConversionUtils::ToVec3(imp.m_vImpulse), ezPxConversionUtils::ToVec3(imp.m_vPos), PxForceMode::eIMPULSE);
    }
  }
}

void ezPxRagdollComponent::DestroyPhysicsShapes()
{
  if (m_pArticulation)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
    EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

    m_pArticulation->release();
    m_pArticulation = nullptr;

    m_pAggregate->release();
    m_pAggregate = nullptr;

    pModule->DeallocateUserData(m_uiUserDataIndex);
    m_uiUserDataIndex = ezInvalidIndex;
  }
}

void ezPxRagdollComponent::UpdatePose()
{
  if (!m_hSkeleton.IsValid() || m_ArticulationLinks.IsEmpty() || m_pRootLink == nullptr)
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();

  ezDynamicArray<ezMat4> poses;
  poses.SetCountUninitialized(m_ArticulationLinks.GetCount());

  ezTransform rt = desc.m_RootTransform;
  ezMat4 mrt = rt.GetAsMat4();
  ezMat4 invRoot = mrt.GetInverse();

  ezMat4 scale;
  scale.SetScalingMatrix(desc.m_RootTransform.m_vScale);

  ezMsgAnimationPoseUpdated poseMsg;
  poseMsg.m_ModelTransforms = poses;
  poseMsg.m_pRootTransform = &rt;
  poseMsg.m_pSkeleton = &desc.m_Skeleton;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  EZ_PX_READ_LOCK(*pModule->GetPxScene());

  const ezTransform newRootTransform = ezPxConversionUtils::ToTransform(m_pRootLink->getGlobalPose());
  GetOwner()->SetGlobalTransform(newRootTransform);

  ezMat4 mInv = invRoot * newRootTransform.GetInverse().GetAsMat4();

  for (ezUInt32 i = 0; i < m_ArticulationLinks.GetCount(); ++i)
  {
    if (m_ArticulationLinks[i].m_pLink == nullptr)
    {
      // TODO: store original pose for this bone ?
      poses[i] = mInv * scale;
    }
    else
    {
      poses[i] = mInv * ezPxConversionUtils::ToTransform(m_ArticulationLinks[i].m_pLink->getGlobalPose()).GetAsMat4() * scale;
    }
  }

  GetOwner()->SendMessage(poseMsg);
}

void ezPxRagdollComponent::Update()
{
  if (m_bShapesCreated)
  {
    UpdatePose();
  }

  //m_Impulses.Clear();
}

void ezPxRagdollComponent::CreateShapesFromBindPose()
{
  if (m_bShapesCreated)
    return;

  {
    ezMsgQueryAnimationSkeleton msg;
    GetOwner()->SendMessage(msg);

    if (!msg.m_hSkeleton.IsValid())
      return;

    m_hSkeleton = msg.m_hSkeleton;
  }

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();

  ezDynamicArray<ezMat4> bones;
  bones.SetCountUninitialized(desc.m_Skeleton.GetJointCount());

  auto getBone = [&](ezUInt32 i, auto f) -> ezMat4 {
    const auto& j = desc.m_Skeleton.GetJointByIndex(i);
    const ezMat4 bm = j.GetBindPoseLocalTransform().GetAsMat4();

    if (j.GetParentIndex() != ezInvalidJointIndex)
    {
      const ezMat4 pbm = f(j.GetParentIndex(), f);

      return pbm * bm;
    }

    return bm;
  };

  for (ezUInt32 i = 0; i < bones.GetCount(); ++i)
  {
    bones[i] = getBone(i, getBone);
  }

  ezMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &desc.m_Skeleton;
  msg.m_ModelTransforms = bones;

  CreatePhysicsShapes(m_hSkeleton, msg);
}

void ezPxRagdollComponent::AddForceAtPos(ezMsgPhysicsAddForce& msg)
{
  if (m_pArticulation != nullptr)
  {
    EZ_PX_WRITE_LOCK(*m_pArticulation->getScene());

    PxArticulationLink* pLink[1] = {};

    if (msg.m_pInternalPhysicsActor != nullptr)
      pLink[0] = reinterpret_cast<PxArticulationLink*>(msg.m_pInternalPhysicsActor);
    else
      m_pArticulation->getLinks(pLink, 1);

    PxRigidBodyExt::addForceAtPos(*pLink[0], ezPxConversionUtils::ToVec3(msg.m_vForce), ezPxConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eFORCE);
  }
}

void ezPxRagdollComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg)
{
  if (!m_bShapesCreated)
  {
    auto& imp = m_Impulses.ExpandAndGetRef();
    imp.m_vPos = msg.m_vGlobalPosition;
    imp.m_vImpulse = msg.m_vImpulse;
    return;
  }

  if (m_pArticulation != nullptr)
  {
    EZ_PX_WRITE_LOCK(*m_pArticulation->getScene());

    PxArticulationLink* pLink[1] = {};

    if (msg.m_pInternalPhysicsActor != nullptr)
      pLink[0] = reinterpret_cast<PxArticulationLink*>(msg.m_pInternalPhysicsActor);
    else
      m_pArticulation->getLinks(pLink, 1);

    PxRigidBodyExt::addForceAtPos(*pLink[0], ezPxConversionUtils::ToVec3(msg.m_vImpulse), ezPxConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eIMPULSE);
  }
}
