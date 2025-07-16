#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSkeletonComponent, 5, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Skeleton", GetSkeleton, SetSkeleton)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    EZ_MEMBER_PROPERTY("VisualizeSkeleton", m_bVisualizeBones)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("VisualizeColliders", m_bVisualizeColliders),
    EZ_MEMBER_PROPERTY("VisualizeJoints", m_bVisualizeJoints),
    EZ_MEMBER_PROPERTY("VisualizeSwingLimits", m_bVisualizeSwingLimits),
    EZ_MEMBER_PROPERTY("VisualizeTwistLimits", m_bVisualizeTwistLimits),
    EZ_ACCESSOR_PROPERTY("BonesToHighlight", GetBonesToHighlight, SetBonesToHighlight),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    EZ_MESSAGE_HANDLER(ezMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSkeletonComponent::ezSkeletonComponent() = default;
ezSkeletonComponent::~ezSkeletonComponent() = default;

ezResult ezSkeletonComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  if (m_MaxBounds.IsValid())
  {
    ezBoundingBox bbox = m_MaxBounds;
    ref_bounds = ezBoundingBoxSphere::MakeFromBox(bbox);
    ref_bounds.Transform(m_RootTransform.GetAsMat4());
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezSkeletonComponent::Update()
{
  if (m_hSkeleton.IsValid() && (m_bVisualizeBones || m_bVisualizeColliders || m_bVisualizeJoints || m_bVisualizeSwingLimits || m_bVisualizeTwistLimits))
  {
    ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);

    if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
      return;

    if (m_uiSkeletonChangeCounter != pSkeleton->GetCurrentResourceChangeCounter())
    {
      VisualizeSkeletonDefaultState();
    }

    const ezQuat qBoneDir = ezBasisAxis::GetBasisRotation_PosX(pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
    const ezVec3 vBoneDir = qBoneDir * ezVec3(1, 0, 0);
    const ezVec3 vBoneTangent = qBoneDir * ezVec3(0, 1, 0);

    ezDebugRenderer::DrawLines(GetWorld(), m_LinesSkeleton, ezColor::White, GetOwner()->GetGlobalTransform());

    for (const auto& shape : m_SpheresShapes)
    {
      ezDebugRenderer::DrawLineSphere(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_BoxShapes)
    {
      ezDebugRenderer::DrawLineBox(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_CapsuleShapes)
    {
      ezDebugRenderer::DrawLineCapsuleZ(GetWorld(), shape.m_fLength, shape.m_fRadius, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_AngleShapes)
    {
      ezDebugRenderer::DrawAngle(GetWorld(), shape.m_StartAngle, shape.m_EndAngle, ezColor::MakeZero(), shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform, vBoneTangent, vBoneDir);
    }

    for (const auto& shape : m_ConeLimitShapes)
    {
      ezDebugRenderer::DrawLimitCone(GetWorld(), shape.m_Angle1, shape.m_Angle2, ezColor::MakeZero(), shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_CylinderShapes)
    {
      ezDebugRenderer::DrawCylinder(GetWorld(), shape.m_fRadius1, shape.m_fRadius2, shape.m_fLength, shape.m_Color, ezColor::MakeZero(), GetOwner()->GetGlobalTransform() * shape.m_Transform, false, false);
    }
  }
}

void ezSkeletonComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_bVisualizeBones;
  s << m_sBonesToHighlight;
  s << m_bVisualizeColliders;
  s << m_bVisualizeJoints;
  s << m_bVisualizeSwingLimits;
  s << m_bVisualizeTwistLimits;
}

void ezSkeletonComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  if (uiVersion <= 4)
    return;

  auto& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_bVisualizeBones;
  s >> m_sBonesToHighlight;
  s >> m_bVisualizeColliders;
  s >> m_bVisualizeJoints;
  s >> m_bVisualizeSwingLimits;
  s >> m_bVisualizeTwistLimits;
}

void ezSkeletonComponent::OnActivated()
{
  SUPER::OnActivated();

  m_MaxBounds = ezBoundingBox::MakeInvalid();
  VisualizeSkeletonDefaultState();
}

void ezSkeletonComponent::SetSkeleton(const ezSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;

    m_MaxBounds = ezBoundingBox::MakeInvalid();
    VisualizeSkeletonDefaultState();
  }
}

void ezSkeletonComponent::SetBonesToHighlight(const char* szFilter)
{
  if (m_sBonesToHighlight != szFilter)
  {
    m_sBonesToHighlight = szFilter;

    m_uiSkeletonChangeCounter = 0xFFFFFFFF;

    VisualizeSkeletonDefaultState();
  }
}

const char* ezSkeletonComponent::GetBonesToHighlight() const
{
  return m_sBonesToHighlight;
}

void ezSkeletonComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg)
{
  m_LinesSkeleton.Clear();
  m_SpheresShapes.Clear();
  m_BoxShapes.Clear();
  m_CapsuleShapes.Clear();
  m_AngleShapes.Clear();
  m_ConeLimitShapes.Clear();
  m_CylinderShapes.Clear();

  m_RootTransform = *msg.m_pRootTransform;

  BuildSkeletonVisualization(msg);
  BuildColliderVisualization(msg);
  BuildJointVisualization(msg);

  ezBoundingBox poseBounds;
  poseBounds = ezBoundingBox::MakeInvalid();

  for (const auto& bone : msg.m_ModelTransforms)
  {
    poseBounds.ExpandToInclude(bone.GetTranslationVector());
  }

  if (poseBounds.IsValid() && (!m_MaxBounds.IsValid() || !m_MaxBounds.Contains(poseBounds)))
  {
    m_MaxBounds.ExpandToInclude(poseBounds);
    TriggerLocalBoundsUpdate();
  }
  else if (((ezRenderWorld::GetFrameCounter() + GetUniqueIdForRendering()) & (EZ_BIT(10) - 1)) == 0) // reset the bbox every once in a while
  {
    m_MaxBounds = poseBounds;
    TriggerLocalBoundsUpdate();
  }
}

void ezSkeletonComponent::BuildSkeletonVisualization(ezMsgAnimationPoseUpdated& msg)
{
  if (!m_bVisualizeBones || !msg.m_pSkeleton)
    return;

  ezStringBuilder tmp;

  struct Bone
  {
    ezVec3 pos = ezVec3::MakeZero();
    ezVec3 dir = ezVec3::MakeZero();
    float distToParent = 0.0f;
    float minDistToChild = 10.0f;
    bool highlight = false;
  };

  ezHybridArray<Bone, 128> bones;

  bones.SetCount(msg.m_pSkeleton->GetJointCount());
  m_LinesSkeleton.Reserve(m_LinesSkeleton.GetCount() + msg.m_pSkeleton->GetJointCount());

  const ezVec3 vBoneDir = ezBasisAxis::GetBasisVector(msg.m_pSkeleton->m_BoneDirection);

  auto renderBone = [&](int iCurrentBone, int iParentBone)
  {
    if (iParentBone == ozz::animation::Skeleton::kNoParent)
      return;

    const ezVec3 v0 = *msg.m_pRootTransform * msg.m_ModelTransforms[iParentBone].GetTranslationVector();
    const ezVec3 v1 = *msg.m_pRootTransform * msg.m_ModelTransforms[iCurrentBone].GetTranslationVector();

    ezVec3 dirToBone = (v1 - v0);

    auto& bone = bones[iCurrentBone];
    bone.pos = v1;
    bone.distToParent = dirToBone.GetLength();
    bone.dir = *msg.m_pRootTransform * msg.m_ModelTransforms[iCurrentBone].TransformDirection(vBoneDir);
    bone.dir.NormalizeIfNotZero(ezVec3::MakeZero()).IgnoreResult();

    auto& pb = bones[iParentBone];

    if (!pb.dir.IsZero() && dirToBone.NormalizeIfNotZero(ezVec3::MakeZero()).Succeeded())
    {
      if (pb.dir.GetAngleBetween(dirToBone) < ezAngle::MakeFromDegree(45))
      {
        ezPlane plane;
        plane = ezPlane::MakeFromNormalAndPoint(pb.dir, pb.pos);
        pb.minDistToChild = ezMath::Min(pb.minDistToChild, plane.GetDistanceTo(v1));
      }
    }
  };

  ozz::animation::IterateJointsDF(msg.m_pSkeleton->GetOzzSkeleton(), renderBone);

  if (m_sBonesToHighlight == "*")
  {
    for (ezUInt32 b = 0; b < bones.GetCount(); ++b)
    {
      bones[b].highlight = true;
    }
  }
  else if (!m_sBonesToHighlight.IsEmpty())
  {
    const ezStringBuilder mask(";", m_sBonesToHighlight, ";");

    for (ezUInt16 b = 0; b < static_cast<ezUInt16>(bones.GetCount()); ++b)
    {
      const ezString currentName = msg.m_pSkeleton->GetJointByIndex(b).GetName().GetString();

      tmp.Set(";", currentName, ";");

      if (mask.FindSubString(tmp))
      {
        bones[b].highlight = true;
      }
    }
  }

  for (ezUInt32 b = 0; b < bones.GetCount(); ++b)
  {
    const auto& bone = bones[b];

    if (!bone.highlight)
    {
      float len = 0.3f;

      if (bone.minDistToChild < 10.0f)
      {
        len = bone.minDistToChild;
      }
      else if (bone.distToParent > 0)
      {
        len = ezMath::Max(bone.distToParent * 0.5f, 0.1f);
      }
      else
      {
        len = 0.1f;
      }

      ezVec3 v0 = bone.pos;
      ezVec3 v1 = bone.pos + bone.dir * len;

      m_LinesSkeleton.PushBack(ezDebugRendererLine(v0, v1));
      m_LinesSkeleton.PeekBack().m_startColor = ezColor::DarkCyan;
      m_LinesSkeleton.PeekBack().m_endColor = ezColor::DarkCyan;
    }
  }

  for (ezUInt32 b = 0; b < bones.GetCount(); ++b)
  {
    const auto& bone = bones[b];

    if (bone.highlight && !bone.dir.IsZero(0.0001f))
    {
      float len = 0.3f;

      if (bone.minDistToChild < 10.0f)
      {
        len = bone.minDistToChild;
      }
      else if (bone.distToParent > 0)
      {
        len = ezMath::Max(bone.distToParent * 0.5f, 0.1f);
      }
      else
      {
        len = 0.1f;
      }

      ezVec3 v0 = bone.pos;
      ezVec3 v1 = bone.pos + bone.dir * len;

      const ezVec3 vO1 = bone.dir.GetOrthogonalVector().GetNormalized();
      const ezVec3 vO2 = bone.dir.CrossRH(vO1).GetNormalized();

      ezVec3 s[4];
      s[0] = v0 + vO1 * len * 0.1f + bone.dir * len * 0.1f;
      s[1] = v0 + vO2 * len * 0.1f + bone.dir * len * 0.1f;
      s[2] = v0 - vO1 * len * 0.1f + bone.dir * len * 0.1f;
      s[3] = v0 - vO2 * len * 0.1f + bone.dir * len * 0.1f;

      m_LinesSkeleton.PushBack(ezDebugRendererLine(v0, v1));
      m_LinesSkeleton.PeekBack().m_startColor = ezColor::DarkCyan;
      m_LinesSkeleton.PeekBack().m_endColor = ezColor::DarkCyan;

      for (ezUInt32 si = 0; si < 4; ++si)
      {
        m_LinesSkeleton.PushBack(ezDebugRendererLine(v0, s[si]));
        m_LinesSkeleton.PeekBack().m_startColor = ezColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor = ezColor::Chartreuse;

        m_LinesSkeleton.PushBack(ezDebugRendererLine(s[si], v1));
        m_LinesSkeleton.PeekBack().m_startColor = ezColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor = ezColor::Chartreuse;
      }
    }
  }
}

void ezSkeletonComponent::BuildColliderVisualization(ezMsgAnimationPoseUpdated& msg)
{
  if (!m_bVisualizeColliders || !msg.m_pSkeleton || !m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  const ezQuat qBoneDirAdjustment = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, srcBoneDir);

  ezStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  ezStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  // the capsule should extend along X, but the debug renderer draws them along Z
  const ezQuat qRotZtoX = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(-90));
  const ezQuat qRotYtoZ = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(90));

  for (const auto& geo : pSkeleton->GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == ezSkeletonJointGeometryType::None)
      continue;

    ezMat4 boneTrans;
    ezQuat boneRot;
    msg.ComputeFullBoneTransform(geo.m_uiAttachedToJoint, boneTrans, boneRot);

    boneName.Set(";", msg.m_pSkeleton->GetJointByIndex(geo.m_uiAttachedToJoint).GetName().GetString(), ";");
    const bool bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindLastSubString(boneName) != nullptr;
    const ezColor hlS = ezMath::Lerp(ezColor::DimGrey, ezColor::Yellow, bHighlight ? 1.0f : 0.2f);

    const ezQuat qFinalBoneRot = boneRot * qBoneDirAdjustment;

    ezTransform st;
    st.SetIdentity();
    st.m_vPosition = boneTrans.GetTranslationVector() + qFinalBoneRot * geo.m_Transform.m_vPosition;
    st.m_qRotation = qFinalBoneRot * geo.m_Transform.m_qRotation;

    if (geo.m_Type == ezSkeletonJointGeometryType::Sphere)
    {
      auto& shape = m_SpheresShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_Shape = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), geo.m_Transform.m_vScale.z);
      shape.m_Color = hlS;
    }

    if (geo.m_Type == ezSkeletonJointGeometryType::Box)
    {
      auto& shape = m_BoxShapes.ExpandAndGetRef();

      ezVec3 ext;
      ext.x = geo.m_Transform.m_vScale.x * 0.5f;
      ext.y = geo.m_Transform.m_vScale.y * 0.5f;
      ext.z = geo.m_Transform.m_vScale.z * 0.5f;

      // TODO: if offset desired
      st.m_vPosition += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      shape.m_Transform = st;
      shape.m_Shape = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3::MakeZero(), ext);
      shape.m_Color = hlS;
    }

    if (geo.m_Type == ezSkeletonJointGeometryType::Capsule)
    {
      st.m_qRotation = st.m_qRotation * qRotZtoX;

      // TODO: if offset desired
      st.m_vPosition += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      auto& shape = m_CapsuleShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_fLength = geo.m_Transform.m_vScale.x;
      shape.m_fRadius = geo.m_Transform.m_vScale.z;
      shape.m_Color = hlS;
    }

    if (geo.m_Type == ezSkeletonJointGeometryType::CapsuleSideways)
    {
      st.m_qRotation = st.m_qRotation * qRotYtoZ;

      auto& shape = m_CapsuleShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_fLength = geo.m_Transform.m_vScale.x;
      shape.m_fRadius = geo.m_Transform.m_vScale.z;
      shape.m_Color = hlS;
    }

    if (geo.m_Type == ezSkeletonJointGeometryType::ConvexMesh)
    {
      st.SetIdentity();
      st = *msg.m_pRootTransform;

      for (ezUInt32 f = 0; f < geo.m_TriangleIndices.GetCount(); f += 3)
      {
        const ezUInt32 i0 = geo.m_TriangleIndices[f + 0];
        const ezUInt32 i1 = geo.m_TriangleIndices[f + 1];
        const ezUInt32 i2 = geo.m_TriangleIndices[f + 2];

        {
          auto& l = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start = st * geo.m_VertexPositions[i0];
          l.m_end = st * geo.m_VertexPositions[i1];
        }
        {
          auto& l = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start = st * geo.m_VertexPositions[i1];
          l.m_end = st * geo.m_VertexPositions[i2];
        }
        {
          auto& l = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start = st * geo.m_VertexPositions[i2];
          l.m_end = st * geo.m_VertexPositions[i0];
        }
      }
    }
  }
}

void ezSkeletonComponent::BuildJointVisualization(ezMsgAnimationPoseUpdated& msg)
{
  if (!m_hSkeleton.IsValid() || (!m_bVisualizeJoints && !m_bVisualizeSwingLimits && !m_bVisualizeTwistLimits))
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& skel = pSkeleton->GetDescriptor().m_Skeleton;

  ezStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  ezStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  const ezQuat qBoneDir = ezBasisAxis::GetBasisRotation_PosX(pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const ezQuat qBoneDirT = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveY, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const ezQuat qBoneDirBT = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveZ, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const ezQuat qBoneDirT2 = ezBasisAxis::GetBasisRotation(ezBasisAxis::NegativeY, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);

  for (ezUInt16 uiJointIdx = 0; uiJointIdx < skel.GetJointCount(); ++uiJointIdx)
  {
    const auto& thisJoint = skel.GetJointByIndex(uiJointIdx);
    const ezUInt16 uiParentIdx = thisJoint.GetParentIndex();

    if (thisJoint.IsRootJoint())
      continue;

    boneName.Set(";", thisJoint.GetName().GetString(), ";");

    const bool bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindSubString(boneName) != nullptr;

    ezMat4 parentTrans;
    ezQuat parentRot; // contains root transform
    msg.ComputeFullBoneTransform(uiParentIdx, parentTrans, parentRot);

    ezMat4 thisTrans; // contains root transform
    ezQuat thisRot;   // contains root transform
    msg.ComputeFullBoneTransform(uiJointIdx, thisTrans, thisRot);

    const ezVec3 vJointPos = thisTrans.GetTranslationVector();
    const ezQuat qLimitRot = parentRot * thisJoint.GetLocalOrientation();

    // main directions
    if (m_bVisualizeJoints && thisJoint.GetJointType() != ezSkeletonJointType::None)
    {
      const ezColor hlM = ezMath::Lerp(ezColor::OrangeRed, ezColor::DimGrey, bHighlight ? 0 : 0.8f);
      const ezColor hlT = ezMath::Lerp(ezColor::LawnGreen, ezColor::DimGrey, bHighlight ? 0 : 0.8f);
      const ezColor hlBT = ezMath::Lerp(ezColor::BlueViolet, ezColor::DimGrey, bHighlight ? 0 : 0.8f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlM;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDir;
        cyl.m_Transform.m_vScale.Set(1);
      }

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirT;
        cyl.m_Transform.m_vScale.Set(1);
      }

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlBT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirBT;
        cyl.m_Transform.m_vScale.Set(1);
      }
    }

    // swing limit
    if (m_bVisualizeSwingLimits && thisJoint.GetJointType() == ezSkeletonJointType::SwingTwist)
    {
      auto& shape = m_ConeLimitShapes.ExpandAndGetRef();
      shape.m_Angle1 = thisJoint.GetHalfSwingLimitY();
      shape.m_Angle2 = thisJoint.GetHalfSwingLimitZ();
      shape.m_Color = ezMath::Lerp(ezColor::DimGrey, ezColor::DeepPink, bHighlight ? 1.0f : 0.2f);
      shape.m_Transform.m_vScale.Set(0.05f);
      shape.m_Transform.m_vPosition = vJointPos;
      shape.m_Transform.m_qRotation = qLimitRot * qBoneDir;

      const ezColor hlM = ezMath::Lerp(ezColor::OrangeRed, ezColor::DimGrey, bHighlight ? 0 : 0.8f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlM;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDir;
        cyl.m_Transform.m_vScale.Set(1);
      }
    }

    // twist limit
    if (m_bVisualizeTwistLimits && thisJoint.GetJointType() == ezSkeletonJointType::SwingTwist)
    {
      auto& shape = m_AngleShapes.ExpandAndGetRef();
      shape.m_StartAngle = thisJoint.GetTwistLimitLow();
      shape.m_EndAngle = thisJoint.GetTwistLimitHigh();
      shape.m_Color = ezMath::Lerp(ezColor::DimGrey, ezColor::LightPink, bHighlight ? 0.8f : 0.2f);
      shape.m_Transform.m_vScale.Set(0.04f);
      shape.m_Transform.m_vPosition = vJointPos;
      shape.m_Transform.m_qRotation = qLimitRot;

      const ezColor hlT = ezMath::Lerp(ezColor::DimGrey, ezColor::LightPink, bHighlight ? 1.0f : 0.4f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirT2;
        cyl.m_Transform.m_vScale.Set(1);

        ezVec3 vDir = cyl.m_Transform.m_qRotation * ezVec3(1, 0, 0);
        vDir.Normalize();

        ezVec3 vDirRef = shape.m_Transform.m_qRotation * qBoneDir * ezVec3(0, 1, 0);
        vDirRef.Normalize();

        const ezVec3 vRotDir = shape.m_Transform.m_qRotation * qBoneDir * ezVec3(1, 0, 0);
        ezQuat qRotRef = ezQuat::MakeFromAxisAndAngle(vRotDir, thisJoint.GetTwistLimitCenterAngle());
        vDirRef = qRotRef * vDirRef;

        // if the current twist is outside the twist limit range, highlight the bone
        if (vDir.GetAngleBetween(vDirRef) > thisJoint.GetTwistLimitHalfAngle())
        {
          cyl.m_Color = ezColor::Orange;
        }
      }
    }
  }
}

void ezSkeletonComponent::VisualizeSkeletonDefaultState()
{
  if (!IsActiveAndInitialized())
    return;

  m_uiSkeletonChangeCounter = 0;

  if (m_hSkeleton.IsValid())
  {
    ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSkeleton.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      m_uiSkeletonChangeCounter = pSkeleton->GetCurrentResourceChangeCounter();

      if (pSkeleton->GetDescriptor().m_Skeleton.GetJointCount() > 0)
      {
        ozz::vector<ozz::math::Float4x4> modelTransforms;
        modelTransforms.resize(pSkeleton->GetDescriptor().m_Skeleton.GetJointCount());

        {
          ozz::animation::LocalToModelJob job;
          job.input = pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();
          job.output = make_span(modelTransforms);
          job.skeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
          job.Run();
        }

        ezMsgAnimationPoseUpdated msg;
        msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
        msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
        msg.m_ModelTransforms = ezArrayPtr<const ezMat4>(reinterpret_cast<const ezMat4*>(&modelTransforms[0]), (ezUInt32)modelTransforms.size());

        OnAnimationPoseUpdated(msg);
      }
    }
  }

  TriggerLocalBoundsUpdate();
}

ezDebugRendererLine& ezSkeletonComponent::AddLine(const ezVec3& vStart, const ezVec3& vEnd, const ezColor& color)
{
  auto& line = m_LinesSkeleton.ExpandAndGetRef();
  line.m_start = vStart;
  line.m_end = vEnd;
  line.m_startColor = color;
  line.m_endColor = color;
  return line;
}

void ezSkeletonComponent::OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg)
{
  // if we have a skeleton, always overwrite it any incoming message with that
  if (m_hSkeleton.IsValid())
  {
    msg.m_hSkeleton = m_hSkeleton;
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonComponent);
