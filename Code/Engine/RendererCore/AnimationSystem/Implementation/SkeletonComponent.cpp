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
EZ_BEGIN_COMPONENT_TYPE(ezSkeletonComponent, 4, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
    EZ_MEMBER_PROPERTY("VisualizeSkeleton", m_bVisualizeSkeleton)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("BonesToHighlight", GetBonesToHighlight, SetBonesToHighlight),
    EZ_MEMBER_PROPERTY("VisualizeColliders", m_bVisualizeColliders),
    EZ_MEMBER_PROPERTY("VisualizeJoints", m_bVisualizeJoints),
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

ezResult ezSkeletonComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_MaxBounds.IsValid())
  {
    ezBoundingBox bbox = m_MaxBounds;
    //bbox.Grow(ezVec3(pMesh->m_fMaxBoneVertexOffset));
    bounds = bbox;
    bounds.Transform(m_RootTransform.GetAsMat4());
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezSkeletonComponent::Update()
{
  if (m_bVisualizeSkeleton || m_bVisualizeColliders || m_bVisualizeJoints)
  {
    if (m_hSkeleton.IsValid())
    {
      ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::PointerOnly);
      if (m_uiSkeletonChangeCounter != pSkeleton->GetCurrentResourceChangeCounter())
      {
        UpdateSkeletonVis();
      }
    }

    ezDebugRenderer::DrawLines(GetWorld(), m_LinesSkeleton, ezColor::White, GetOwner()->GetGlobalTransform());

    for (const auto& shape : m_SpheresSkeleton)
    {
      ezDebugRenderer::DrawLineSphere(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_BoxesSkeleton)
    {
      ezDebugRenderer::DrawLineBox(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_CapsulesSkeleton)
    {
      ezDebugRenderer::DrawLineCapsuleZ(GetWorld(), shape.m_fLength, shape.m_fRadius, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }
  }
}

void ezSkeletonComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hSkeleton;
  s << m_bVisualizeSkeleton;
  s << m_sBonesToHighlight;
  s << m_bVisualizeColliders;
  s << m_bVisualizeJoints;
}

void ezSkeletonComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hSkeleton;
  s >> m_bVisualizeSkeleton;

  if (uiVersion >= 2)
  {
    s >> m_sBonesToHighlight;
  }

  if (uiVersion >= 3)
  {
    s >> m_bVisualizeColliders;
  }

  if (uiVersion >= 4)
  {
    s >> m_bVisualizeJoints;
  }
}

void ezSkeletonComponent::OnActivated()
{
  SUPER::OnActivated();

  m_MaxBounds.SetInvalid();
  UpdateSkeletonVis();
}

void ezSkeletonComponent::SetSkeletonFile(const char* szFile)
{
  ezSkeletonResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezSkeletonResource>(szFile);
  }

  SetSkeleton(hResource);
}

const char* ezSkeletonComponent::GetSkeletonFile() const
{
  if (!m_hSkeleton.IsValid())
    return "";

  return m_hSkeleton.GetResourceID();
}


void ezSkeletonComponent::SetSkeleton(const ezSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;

    m_MaxBounds.SetInvalid();
    UpdateSkeletonVis();
  }
}

void ezSkeletonComponent::SetBonesToHighlight(const char* szFilter)
{
  if (m_sBonesToHighlight != szFilter)
  {
    m_sBonesToHighlight = szFilter;

    m_uiSkeletonChangeCounter = 0xFFFFFFFF;

    UpdateSkeletonVis();
  }
}

const char* ezSkeletonComponent::GetBonesToHighlight() const
{
  return m_sBonesToHighlight;
}

void ezSkeletonComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg)
{
  m_LinesSkeleton.Clear();
  m_SpheresSkeleton.Clear();
  m_BoxesSkeleton.Clear();
  m_CapsulesSkeleton.Clear();

  m_RootTransform = *msg.m_pRootTransform;

  ezBoundingBox poseBounds;
  poseBounds.SetInvalid();

  BuildSkeletonVisualization(msg, poseBounds);
  BuildColliderVisualization(msg);
  BuildJointVisualization(msg);

  if (!m_MaxBounds.IsValid() || !m_MaxBounds.Contains(poseBounds))
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

void ezSkeletonComponent::BuildSkeletonVisualization(ezMsgAnimationPoseUpdated& msg, ezBoundingBox& bounds)
{
  for (const auto& bone : msg.m_ModelTransforms)
  {
    bounds.ExpandToInclude(bone.GetTranslationVector());
  }

  if (!m_bVisualizeSkeleton || !msg.m_pSkeleton)
    return;

  ezStringBuilder tmp;

  struct Bone
  {
    ezVec3 pos = ezVec3::ZeroVector();
    ezVec3 dir = ezVec3::ZeroVector();
    float distToParent = 0.0f;
    float minDistToChild = 10.0f;
    bool highlight = false;
  };

  ezHybridArray<Bone, 128> bones;

  bones.SetCount(msg.m_pSkeleton->GetJointCount());
  m_LinesSkeleton.Reserve(m_LinesSkeleton.GetCount() + msg.m_pSkeleton->GetJointCount());

  const ezVec3 vBoneDir = ezBasisAxis::GetBasisVector(msg.m_pSkeleton->m_BoneDirection);

  auto renderBone = [&](int currentBone, int parentBone) {
    if (parentBone == ozz::animation::Skeleton::kNoParent)
      return;

    const ezVec3 v0 = *msg.m_pRootTransform * msg.m_ModelTransforms[parentBone].GetTranslationVector();
    const ezVec3 v1 = *msg.m_pRootTransform * msg.m_ModelTransforms[currentBone].GetTranslationVector();

    ezVec3 dirToBone = (v1 - v0);

    auto& bone = bones[currentBone];
    bone.pos = v1;
    bone.distToParent = dirToBone.GetLength();
    bone.dir = *msg.m_pRootTransform * msg.m_ModelTransforms[currentBone].TransformDirection(vBoneDir);
    bone.dir.NormalizeIfNotZero(ezVec3::ZeroVector()).IgnoreResult();

    auto& pb = bones[parentBone];

    if (!pb.dir.IsZero() && dirToBone.NormalizeIfNotZero(ezVec3::ZeroVector()).Succeeded())
    {
      if (pb.dir.GetAngleBetween(dirToBone) < ezAngle::Degree(45))
      {
        ezPlane plane;
        plane.SetFromNormalAndPoint(pb.dir, pb.pos);
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

    for (ezUInt32 b = 0; b < bones.GetCount(); ++b)
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

      m_LinesSkeleton.PushBack(ezDebugRenderer::Line(v0, v1));
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

      m_LinesSkeleton.PushBack(ezDebugRenderer::Line(v0, v1));
      m_LinesSkeleton.PeekBack().m_startColor = ezColor::DarkCyan;
      m_LinesSkeleton.PeekBack().m_endColor = ezColor::DarkCyan;

      for (ezUInt32 si = 0; si < 4; ++si)
      {
        m_LinesSkeleton.PushBack(ezDebugRenderer::Line(v0, s[si]));
        m_LinesSkeleton.PeekBack().m_startColor = ezColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor = ezColor::Chartreuse;

        m_LinesSkeleton.PushBack(ezDebugRenderer::Line(s[si], v1));
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

  ezStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  ezStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  for (const auto& geo : pSkeleton->GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == ezSkeletonJointGeometryType::None)
      continue;

    ezMat4 boneTrans;
    ezQuat boneRot;
    msg.ComputeFullBoneTransform(geo.m_uiAttachedToJoint, boneTrans, boneRot);

    boneName.Set(";", msg.m_pSkeleton->GetJointByIndex(geo.m_uiAttachedToJoint).GetName().GetString(), ";");
    const bool bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindLastSubString(boneName) != nullptr;
    const ezColor hlS = ezMath::Lerp(ezColor::DimGray, ezColor::Yellow, bHighlight ? 1 : 0.2f);

    ezTransform st;
    st.SetIdentity();
    st.m_vPosition = boneTrans.TransformPosition(geo.m_Transform.m_qRotation * geo.m_Transform.m_vPosition);
    st.m_qRotation = boneRot * geo.m_Transform.m_qRotation;

    if (geo.m_Type == ezSkeletonJointGeometryType::Sphere)
    {
      auto& shape = m_SpheresSkeleton.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_Shape.SetElements(ezVec3::ZeroVector(), geo.m_Transform.m_vScale.z);
      shape.m_Color = hlS;
    }

    if (geo.m_Type == ezSkeletonJointGeometryType::Box)
    {
      auto& shape = m_BoxesSkeleton.ExpandAndGetRef();

      ezVec3 ext;
      ext.x = geo.m_Transform.m_vScale.y * 0.5f;
      ext.y = geo.m_Transform.m_vScale.x * 0.5f;
      ext.z = geo.m_Transform.m_vScale.z * 0.5f;

      st.m_vPosition = boneTrans.TransformPosition(geo.m_Transform.m_qRotation * (geo.m_Transform.m_vPosition + ezVec3(0, geo.m_Transform.m_vScale.x * 0.5f, 0)));

      shape.m_Transform = st;
      shape.m_Shape.SetCenterAndHalfExtents(ezVec3::ZeroVector(), ext);
      shape.m_Color = hlS;
    }

    if (geo.m_Type == ezSkeletonJointGeometryType::Capsule)
    {
      ezQuat qRot;
      qRot.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(90));

      st.m_vPosition = boneTrans.TransformPosition(geo.m_Transform.m_qRotation * (geo.m_Transform.m_vPosition + ezVec3(0, geo.m_Transform.m_vScale.x * 0.5f, 0)));

      auto& shape = m_CapsulesSkeleton.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_Transform.m_qRotation = shape.m_Transform.m_qRotation * qRot;
      shape.m_fLength = geo.m_Transform.m_vScale.x;
      shape.m_fRadius = geo.m_Transform.m_vScale.z;
      shape.m_Color = hlS;
    }
  }
}

void ezSkeletonComponent::BuildJointVisualization(ezMsgAnimationPoseUpdated& msg)
{
  if (!m_bVisualizeJoints || !m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& skel = pSkeleton->GetDescriptor().m_Skeleton;

  ezStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  ezStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  for (ezUInt16 uiJointIdx = 0; uiJointIdx < skel.GetJointCount(); ++uiJointIdx)
  {
    const auto& thisJoint = skel.GetJointByIndex(uiJointIdx);
    const ezUInt16 uiParentIdx = thisJoint.GetParentIndex();

    if (uiParentIdx == ezInvalidJointIndex)
      continue;

    boneName.Set(";", thisJoint.GetName().GetString(), ";");

    const bool bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindSubString(boneName) != nullptr;

    ezMat4 parentTrans;
    ezQuat parentRot;
    msg.ComputeFullBoneTransform(uiParentIdx, parentTrans, parentRot);

    ezMat4 thisTrans;
    msg.ComputeFullBoneTransform(uiJointIdx, thisTrans);

    const ezVec3 vJointPos = thisTrans.GetTranslationVector();
    const ezQuat qLimitRot = parentRot * thisJoint.GetLimitRotation();

    // main direction
    {
      const ezColor hlM = ezMath::Lerp(ezColor::BlueViolet, ezColor::DimGrey, bHighlight ? 0 : 0.8f);
      AddLine(vJointPos, vJointPos + qLimitRot * ezVec3(0, 0, 0.12), hlM);
    }

    // swing limit
    {
      ezQuat qRotX, qRotY;
      qRotX.SetFromAxisAndAngle(ezVec3::UnitXAxis(), thisJoint.GetHalfSwingLimitX());
      qRotY.SetFromAxisAndAngle(ezVec3::UnitYAxis(), thisJoint.GetHalfSwingLimitY());

      const ezVec3 endX1 = vJointPos + qLimitRot * qRotX * ezVec3(0, 0, 0.1f);
      const ezVec3 endX2 = vJointPos + qLimitRot * -qRotX * ezVec3(0, 0, 0.1f);
      const ezVec3 endY1 = vJointPos + qLimitRot * qRotY * ezVec3(0, 0, 0.1f);
      const ezVec3 endY2 = vJointPos + qLimitRot * -qRotY * ezVec3(0, 0, 0.1f);

      const ezColor hlX = ezMath::Lerp(ezColor::Red, ezColor::DimGrey, bHighlight ? 0 : 0.8f);
      const ezColor hlY = ezMath::Lerp(ezColor::Lime, ezColor::DimGrey, bHighlight ? 0 : 0.8f);

      AddLine(vJointPos, endX1, hlX);
      AddLine(vJointPos, endX2, hlX);
      AddLine(vJointPos, endY1, hlY);
      AddLine(vJointPos, endY2, hlY);

      AddLine(endX1, ezMath::Lerp(endX1, endY1, 0.5f), hlX);
      AddLine(endX1, ezMath::Lerp(endX1, endY2, 0.5f), hlX);
      AddLine(endX2, ezMath::Lerp(endX2, endY1, 0.5f), hlX);
      AddLine(endX2, ezMath::Lerp(endX2, endY2, 0.5f), hlX);

      AddLine(endY1, ezMath::Lerp(endY1, endX1, 0.5f), hlY);
      AddLine(endY1, ezMath::Lerp(endY1, endX2, 0.5f), hlY);
      AddLine(endY2, ezMath::Lerp(endY2, endX1, 0.5f), hlY);
      AddLine(endY2, ezMath::Lerp(endY2, endX2, 0.5f), hlY);
    }
  }
}

void ezSkeletonComponent::UpdateSkeletonVis()
{
  if (!IsActiveAndInitialized())
    return;

  m_LinesSkeleton.Clear();
  m_SpheresSkeleton.Clear();
  m_BoxesSkeleton.Clear();
  m_CapsulesSkeleton.Clear();
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

ezDebugRenderer::Line& ezSkeletonComponent::AddLine(const ezVec3& vStart, const ezVec3& vEnd, const ezColor& color)
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
