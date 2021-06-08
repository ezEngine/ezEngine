#include <RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSkeletonComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
    EZ_MEMBER_PROPERTY("VisualizeSkeleton", m_bVisualizeSkeleton)->AddAttributes(new ezDefaultValueAttribute(true)),
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

ezResult ezSkeletonComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  bounds = m_LocalBounds;
  return EZ_SUCCESS;
}

void ezSkeletonComponent::Update()
{
  if (m_bVisualizeSkeleton)
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
  }
}

void ezSkeletonComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hSkeleton;
  s << m_bVisualizeSkeleton;
  s << m_sBonesToHighlight;
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
}

void ezSkeletonComponent::OnActivated()
{
  SUPER::OnActivated();

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

  ezBoundingSphere bsphere;
  bsphere.SetInvalid();
  bsphere.m_fRadius = 0.0f;

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

  const ezVec3 vBoneDir = ezBasisAxis::GetBasisVector(msg.m_pSkeleton->m_BoneDirection);

  auto renderBone = [&](int currentBone, int parentBone) {
    if (parentBone == ozz::animation::Skeleton::kNoParent)
      return;

    const ezVec3 v0 = *msg.m_pRootTransform * msg.m_ModelTransforms[parentBone].GetTranslationVector();
    const ezVec3 v1 = *msg.m_pRootTransform * msg.m_ModelTransforms[currentBone].GetTranslationVector();

    ezVec3 dirToBone = (v1 - v0);

    bsphere.ExpandToInclude(v0);

    auto& bone = bones[currentBone];
    bone.pos = v1;
    bone.distToParent = dirToBone.GetLength();
    bone.dir = *msg.m_pRootTransform * msg.m_ModelTransforms[currentBone].TransformDirection(vBoneDir);
    bone.dir.NormalizeIfNotZero(ezVec3::ZeroVector()).IgnoreResult();

    auto& pb = bones[parentBone];

    const ezString currentName = msg.m_pSkeleton->GetJointByIndex(currentBone).GetName().GetString();

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

  // if the existing bounds are big enough, don't update them
  if (!m_LocalBounds.IsValid() || !m_LocalBounds.GetSphere().Contains(bsphere))
  {
    m_LocalBounds.SetInvalid();
    m_LocalBounds.ExpandToInclude(bsphere);

    TriggerLocalBoundsUpdate();
  }
}

void ezSkeletonComponent::UpdateSkeletonVis()
{
  if (!IsActiveAndInitialized())
    return;

  m_LinesSkeleton.Clear();
  m_LocalBounds.SetInvalid();
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
          job.input = pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_bind_poses();
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

void ezSkeletonComponent::OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg)
{
  // if we have a skeleton, always overwrite it any incoming message with that
  if (m_hSkeleton.IsValid())
  {
    msg.m_hSkeleton = m_hSkeleton;
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonComponent);
