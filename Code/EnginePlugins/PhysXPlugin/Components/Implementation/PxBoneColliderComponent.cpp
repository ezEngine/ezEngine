#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxBoneColliderComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Components/PxQueryShapeActorComponent.h>
#include <PhysXPlugin/Shapes/PxShapeBoxComponent.h>
#include <PhysXPlugin/Shapes/PxShapeCapsuleComponent.h>
#include <PhysXPlugin/Shapes/PxShapeSphereComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxBoneColliderComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("QueryShapeOnly", m_bQueryShapeOnly)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("UpdateThreshold", m_UpdateThreshold),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetShapeId),
    EZ_SCRIPT_FUNCTION_PROPERTY(RecreatePhysicsShapes),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/PhysX/Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxBoneColliderComponent::ezPxBoneColliderComponent() = default;
ezPxBoneColliderComponent::~ezPxBoneColliderComponent() = default;

void ezPxBoneColliderComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bQueryShapeOnly;
  s << m_UpdateThreshold;
}

void ezPxBoneColliderComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bQueryShapeOnly;
  s >> m_UpdateThreshold;
}

void ezPxBoneColliderComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  RecreatePhysicsShapes();
}

void ezPxBoneColliderComponent::OnDeactivated()
{
  DestroyPhysicsShapes();

  SUPER::OnDeactivated();
}

void ezPxBoneColliderComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& ref_msg)
{
  if (m_UpdateThreshold.IsPositive())
  {
    const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

    if (tNow - m_LastUpdate < m_UpdateThreshold)
      return;

    m_LastUpdate = tNow;
  }

  for (const auto& shape : m_Shapes)
  {
    ezMat4 boneTrans;
    ezQuat boneRot;
    ref_msg.ComputeFullBoneTransform(shape.m_uiAttachedToBone, boneTrans, boneRot);

    ezTransform pose;
    pose.SetIdentity();
    pose.m_vPosition = boneTrans.GetTranslationVector() + boneRot * shape.m_vOffsetPos;
    pose.m_qRotation = boneRot * shape.m_qOffsetRot;

    ezGameObject* pGO = nullptr;
    if (GetWorld()->TryGetObject(shape.m_hActorObject, pGO))
    {
      pGO->SetLocalPosition(pose.m_vPosition);
      pGO->SetLocalRotation(pose.m_qRotation);
    }
  }
}

void ezPxBoneColliderComponent::RecreatePhysicsShapes()
{
  ezMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  DestroyPhysicsShapes();
  CreatePhysicsShapes(msg.m_hSkeleton);

  m_LastUpdate = ezTime::MakeZero();
}

void ezPxBoneColliderComponent::CreatePhysicsShapes(const ezSkeletonResourceHandle& hSkeleton)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  // ezResourceLock<ezSkeletonResource> pSkeleton(hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  // const auto& desc = pSkeleton->GetDescriptor();

  // EZ_ASSERT_DEV(m_Shapes.IsEmpty(), "");
  // m_Shapes.Reserve(desc.m_Geometry.GetCount());

  // ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  // m_uiShapeID = pModule->CreateShapeId();

  // const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  // const ezQuat qBoneDirAdjustment = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, srcBoneDir);

  // const ezQuat qFinalBoneRot = /*boneRot **/ qBoneDirAdjustment;

  // ezQuat qRotZtoX; // the capsule should extend along X, but the capsule shape goes along Z
  // qRotZtoX.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(-90));

  // for (ezUInt32 idx = 0; idx < desc.m_Geometry.GetCount(); ++idx)
  //{
  //   const auto& geo = desc.m_Geometry[idx];

  //  if (geo.m_Type == ezSkeletonJointGeometryType::None)
  //    continue;

  //  auto& shape = m_Shapes.ExpandAndGetRef();

  //  ezGameObject* pGO = nullptr;

  //  {
  //    ezGameObjectDesc god;
  //    god.m_bDynamic = true;
  //    god.m_hParent = GetOwner()->GetHandle();
  //    god.m_sName = geo.m_sName;
  //    god.m_uiTeamID = GetOwner()->GetTeamID();

  //    shape.m_hActorObject = GetWorld()->CreateObject(god, pGO);

  //    if (m_bQueryShapeOnly)
  //    {
  //      ezPxQueryShapeActorComponent* pDynAct = nullptr;
  //      ezPxQueryShapeActorComponent::CreateComponent(pGO, pDynAct);
  //    }
  //    else
  //    {
  //      ezPxDynamicActorComponent* pDynAct = nullptr;
  //      ezPxDynamicActorComponent::CreateComponent(pGO, pDynAct);
  //      pDynAct->SetKinematic(true);
  //    }
  //  }

  //  shape.m_uiAttachedToBone = geo.m_uiAttachedToJoint;
  //  shape.m_vOffsetPos = /*boneTrans.GetTranslationVector() +*/ qFinalBoneRot * geo.m_Transform.m_vPosition;
  //  shape.m_qOffsetRot = qFinalBoneRot * geo.m_Transform.m_qRotation;


  //  ezPxShapeComponent* pShape = nullptr;

  //  if (geo.m_Type == ezSkeletonJointGeometryType::Sphere)
  //  {
  //    ezPxShapeSphereComponent* pShapeComp = nullptr;
  //    ezPxShapeSphereComponent::CreateComponent(pGO, pShapeComp);
  //    pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
  //    pShape = pShapeComp;
  //  }
  //  else if (geo.m_Type == ezSkeletonJointGeometryType::Box)
  //  {
  //    ezVec3 ext;
  //    ext.x = geo.m_Transform.m_vScale.x;
  //    ext.y = geo.m_Transform.m_vScale.y;
  //    ext.z = geo.m_Transform.m_vScale.z;

  //    // TODO: if offset desired
  //    shape.m_vOffsetPos += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

  //    ezPxShapeBoxComponent* pShapeComp = nullptr;
  //    ezPxShapeBoxComponent::CreateComponent(pGO, pShapeComp);
  //    pShapeComp->SetExtents(ext);
  //    pShape = pShapeComp;
  //  }
  //  else if (geo.m_Type == ezSkeletonJointGeometryType::Capsule)
  //  {
  //    shape.m_qOffsetRot = shape.m_qOffsetRot * qRotZtoX;

  //    // TODO: if offset desired
  //    shape.m_vOffsetPos += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

  //    ezPxShapeCapsuleComponent* pShapeComp = nullptr;
  //    ezPxShapeCapsuleComponent::CreateComponent(pGO, pShapeComp);
  //    pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
  //    pShapeComp->SetHeight(geo.m_Transform.m_vScale.x);
  //    pShape = pShapeComp;
  //  }
  //  else
  //  {
  //    EZ_ASSERT_NOT_IMPLEMENTED;
  //  }

  //  pShape->SetInitialShapeId(m_uiShapeID);
  ////pShape->m_uiCollisionLayer = geo.m_uiCollisionLayer;
  ////pShape->m_hSurface = geo.m_hSurface;
  //}
}

void ezPxBoneColliderComponent::DestroyPhysicsShapes()
{
  for (auto& shape : m_Shapes)
  {
    GetWorld()->DeleteObjectDelayed(shape.m_hActorObject);
  }

  m_Shapes.Clear();
}
