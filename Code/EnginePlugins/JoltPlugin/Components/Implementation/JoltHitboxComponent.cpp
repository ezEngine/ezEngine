#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Components/JoltHitboxComponent.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/Shapes/JoltShapeCapsuleComponent.h>
#include <JoltPlugin/Shapes/JoltShapeSphereComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltHitboxComponent, 2, ezComponentMode::Dynamic)
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
    EZ_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
    EZ_SCRIPT_FUNCTION_PROPERTY(RecreatePhysicsShapes),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltHitboxComponent::ezJoltHitboxComponent() = default;
ezJoltHitboxComponent::~ezJoltHitboxComponent() = default;

void ezJoltHitboxComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bQueryShapeOnly;
  s << m_UpdateThreshold;
}

void ezJoltHitboxComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bQueryShapeOnly;
  s >> m_UpdateThreshold;
}

void ezJoltHitboxComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  RecreatePhysicsShapes();
}

void ezJoltHitboxComponent::OnDeactivated()
{
  if (m_uiObjectFilterID != ezInvalidIndex)
  {
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }

  DestroyPhysicsShapes();

  SUPER::OnDeactivated();
}

void ezJoltHitboxComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& ref_msg)
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

void ezJoltHitboxComponent::RecreatePhysicsShapes()
{
  ezMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  DestroyPhysicsShapes();
  CreatePhysicsShapes(msg.m_hSkeleton);

  m_LastUpdate = ezTime::MakeZero();
}

void ezJoltHitboxComponent::CreatePhysicsShapes(const ezSkeletonResourceHandle& hSkeleton)
{
  ezResourceLock<ezSkeletonResource> pSkeleton(hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  const auto& desc = pSkeleton->GetDescriptor();

  EZ_ASSERT_DEV(m_Shapes.IsEmpty(), "");
  m_Shapes.Reserve(desc.m_Geometry.GetCount());

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  m_uiObjectFilterID = pModule->CreateObjectFilterID();

  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  const ezQuat qBoneDirAdjustment = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, srcBoneDir);

  const ezQuat qFinalBoneRot = /*boneRot **/ qBoneDirAdjustment;

  // the capsule should extend along X, but the capsule shape goes along Z
  const ezQuat qRotZtoX = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(-90));
  const ezQuat qRotYtoZ = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(90));

  for (ezUInt32 idx = 0; idx < desc.m_Geometry.GetCount(); ++idx)
  {
    const auto& geo = desc.m_Geometry[idx];

    if (geo.m_Type == ezSkeletonJointGeometryType::None)
      continue;

    const ezSkeletonJoint& joint = desc.m_Skeleton.GetJointByIndex(geo.m_uiAttachedToJoint);

    auto& shape = m_Shapes.ExpandAndGetRef();

    ezGameObject* pGO = nullptr;

    {
      ezGameObjectDesc god;
      god.m_bDynamic = true;
      god.m_hParent = GetOwner()->GetHandle();
      god.m_sName = joint.GetName();
      god.m_uiTeamID = GetOwner()->GetTeamID();

      shape.m_hActorObject = GetWorld()->CreateObject(god, pGO);

      if (m_bQueryShapeOnly)
      {
        ezJoltQueryShapeActorComponent* pDynAct = nullptr;
        ezJoltQueryShapeActorComponent::CreateComponent(pGO, pDynAct);

        pDynAct->m_uiCollisionLayer = joint.GetCollisionLayer();
        pDynAct->m_hSurface = joint.GetSurface();
        pDynAct->SetInitialObjectFilterID(m_uiObjectFilterID);
      }
      else
      {
        ezJoltDynamicActorComponent* pDynAct = nullptr;
        ezJoltDynamicActorComponent::CreateComponent(pGO, pDynAct);
        pDynAct->SetKinematic(true);

        pDynAct->m_uiCollisionLayer = joint.GetCollisionLayer();
        pDynAct->m_hSurface = joint.GetSurface();
        pDynAct->SetInitialObjectFilterID(m_uiObjectFilterID);
      }
    }

    shape.m_uiAttachedToBone = geo.m_uiAttachedToJoint;
    shape.m_vOffsetPos = /*boneTrans.GetTranslationVector() +*/ qFinalBoneRot * geo.m_Transform.m_vPosition;
    shape.m_qOffsetRot = qFinalBoneRot * geo.m_Transform.m_qRotation;


    if (geo.m_Type == ezSkeletonJointGeometryType::Sphere)
    {
      ezJoltShapeSphereComponent* pShapeComp = nullptr;
      ezJoltShapeSphereComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
    }
    else if (geo.m_Type == ezSkeletonJointGeometryType::Box)
    {
      ezVec3 ext;
      ext.x = geo.m_Transform.m_vScale.x;
      ext.y = geo.m_Transform.m_vScale.y;
      ext.z = geo.m_Transform.m_vScale.z;

      // TODO: if offset desired
      shape.m_vOffsetPos += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      ezJoltShapeBoxComponent* pShapeComp = nullptr;
      ezJoltShapeBoxComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetHalfExtents(ext * 0.5f);
    }
    else if (geo.m_Type == ezSkeletonJointGeometryType::Capsule)
    {
      shape.m_qOffsetRot = shape.m_qOffsetRot * qRotZtoX;

      // TODO: if offset desired
      shape.m_vOffsetPos += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      ezJoltShapeCapsuleComponent* pShapeComp = nullptr;
      ezJoltShapeCapsuleComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
      pShapeComp->SetHeight(geo.m_Transform.m_vScale.x);
    }
    else if (geo.m_Type == ezSkeletonJointGeometryType::CapsuleSideways)
    {
      shape.m_qOffsetRot = shape.m_qOffsetRot * qRotYtoZ;

      ezJoltShapeCapsuleComponent* pShapeComp = nullptr;
      ezJoltShapeCapsuleComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
      pShapeComp->SetHeight(geo.m_Transform.m_vScale.x);
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void ezJoltHitboxComponent::DestroyPhysicsShapes()
{
  for (auto& shape : m_Shapes)
  {
    GetWorld()->DeleteObjectDelayed(shape.m_hActorObject);
  }

  m_Shapes.Clear();
}

EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltHitboxComponent);
