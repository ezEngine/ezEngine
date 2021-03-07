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
EZ_BEGIN_COMPONENT_TYPE(ezSkeletonComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
    EZ_MEMBER_PROPERTY("VisualizeSkeleton", m_bVisualizeSkeleton)->AddAttributes(new ezDefaultValueAttribute(true)),
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
}

void ezSkeletonComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hSkeleton;
  s >> m_bVisualizeSkeleton;
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
    ezStringBuilder tmp(";", szFilter, ";");

    m_sBonesToHighlight = tmp;

    m_uiSkeletonChangeCounter = 0xFFFFFFFF;
  }
}

void ezSkeletonComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg)
{
  m_LinesSkeleton.Clear();

  ezBoundingSphere bsphere;
  bsphere.SetInvalid();
  bsphere.m_fRadius = 0.0f;

  ezStringBuilder tmp;

  auto renderBone = [&](int currentBone, int parentBone) {
    if (parentBone == ozz::animation::Skeleton::kNoParent)
      return;

    const ezVec3 v0 = *msg.m_pRootTransform * msg.m_ModelTransforms[parentBone].GetTranslationVector();
    const ezVec3 v1 = *msg.m_pRootTransform * msg.m_ModelTransforms[currentBone].GetTranslationVector();

    bsphere.ExpandToInclude(v0);

    m_LinesSkeleton.PushBack(ezDebugRenderer::Line(v0, v1));
    m_LinesSkeleton.PeekBack().m_startColor = ezColor::DarkCyan;
    m_LinesSkeleton.PeekBack().m_endColor = ezColor::DarkCyan;

    if (!m_sBonesToHighlight.IsEmpty())
    {
      const ezString parentName = msg.m_pSkeleton->GetJointByIndex(parentBone).GetName().GetString();
      const ezString currentName = msg.m_pSkeleton->GetJointByIndex(currentBone).GetName().GetString();

      tmp.Set(";", currentName, ";");

      if (m_sBonesToHighlight.FindSubString(tmp))
      {
        m_LinesSkeleton.PeekBack().m_startColor = ezColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor = ezColor::Chartreuse;
      }
    }
  };

  ozz::animation::IterateJointsDF(msg.m_pSkeleton->GetOzzSkeleton(), renderBone);

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
