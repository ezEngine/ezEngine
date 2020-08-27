#include <RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/VisualizeSkeletonComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVisualizeSkeletonComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated)
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

ezVisualizeSkeletonComponent::ezVisualizeSkeletonComponent() = default;
ezVisualizeSkeletonComponent::~ezVisualizeSkeletonComponent() = default;

ezResult ezVisualizeSkeletonComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  bounds = m_LocalBounds;
  return EZ_SUCCESS;
}

void ezVisualizeSkeletonComponent::Update()
{
  if (!m_LinesSkeleton.IsEmpty())
  {
    ezDebugRenderer::DrawLines(GetWorld(), m_LinesSkeleton, ezColor::DeepPink, GetOwner()->GetGlobalTransform());
  }

  //if (!m_hSkeleton.IsValid())
  //  return;

  //ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);

  //if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
  //  return;

  //const ezSkeletonResourceDescriptor& desc = pSkeleton->GetDescriptor();
  //const ezSkeleton& skeleton = desc.m_Skeleton;

  //ezAnimationPose pose;
  //pose.Configure(skeleton);
  //pose.SetToBindPoseInLocalSpace(skeleton);
  //pose.ConvertFromLocalSpaceToObjectSpace(skeleton);

  //pose.VisualizePose(GetWorld(), skeleton, GetOwner()->GetGlobalTransform(), 0);

  //ezQuat qRotCapsule;
  //qRotCapsule.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));

  //for (const auto& geo : desc.m_Geometry)
  //{
  //  const ezVec3 scale = geo.m_Transform.m_vScale;

  //  ezTransform geoPose;
  //  geoPose.SetFromMat4(pose.GetTransform(geo.m_uiAttachedToJoint));
  //  geoPose = geoPose * ezTransform(geo.m_Transform.m_vPosition, geo.m_Transform.m_qRotation); // remove scale

  //  switch (geo.m_Type)
  //  {
  //    case ezSkeletonJointGeometryType::None:
  //      break;

  //    case ezSkeletonJointGeometryType::Box:
  //    {
  //      ezBoundingBox box;
  //      box.SetCenterAndHalfExtents(ezVec3::ZeroVector(), scale);
  //      ezDebugRenderer::DrawLineBox(GetWorld(), box, ezColor::IndianRed, geoPose);
  //      break;
  //    }

  //    case ezSkeletonJointGeometryType::Capsule:
  //    {
  //      geoPose.m_qRotation = geoPose.m_qRotation * qRotCapsule;
  //      ezDebugRenderer::DrawLineCapsuleZ(GetWorld(), scale.x, scale.z, ezColor::IndianRed, geoPose);
  //      break;
  //    }

  //    case ezSkeletonJointGeometryType::Sphere:
  //    {
  //      ezBoundingSphere sphere(ezVec3::ZeroVector(), scale.z);
  //      ezDebugRenderer::DrawLineSphere(GetWorld(), sphere, ezColor::IndianRed, geoPose);
  //      break;
  //    }
  //  }
  //}
}

void ezVisualizeSkeletonComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hSkeleton;
}

void ezVisualizeSkeletonComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hSkeleton;
}

void ezVisualizeSkeletonComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  UpdateSkeletonVis();
}

void ezVisualizeSkeletonComponent::SetSkeletonFile(const char* szFile)
{
  ezSkeletonResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezSkeletonResource>(szFile);
  }

  SetSkeleton(hResource);
}

const char* ezVisualizeSkeletonComponent::GetSkeletonFile() const
{
  if (!m_hSkeleton.IsValid())
    return "";

  return m_hSkeleton.GetResourceID();
}


void ezVisualizeSkeletonComponent::SetSkeleton(const ezSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;
    UpdateSkeletonVis();
  }
}

void ezVisualizeSkeletonComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg)
{
  m_LinesSkeleton.Clear();

  ezBoundingSphere bsphere;
  bsphere.SetInvalid();
  bsphere.m_fRadius = 0.0f;

  auto renderBone = [&](int currentBone, int parentBone) {
    if (parentBone == ozz::animation::Skeleton::kNoParent)
      return;

    const ezVec3 v0 = msg.m_ModelTransforms[parentBone].GetTranslationVector();
    const ezVec3 v1 = msg.m_ModelTransforms[currentBone].GetTranslationVector();

    bsphere.ExpandToInclude(v0);

    m_LinesSkeleton.PushBack(ezDebugRenderer::Line(v0, v1));
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

void ezVisualizeSkeletonComponent::UpdateSkeletonVis()
{
  m_LinesSkeleton.Clear();
  m_LocalBounds.SetInvalid();

  if (m_hSkeleton.IsValid())
  {
    ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSkeleton.GetAcquireResult() == ezResourceAcquireResult::Final)
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
      msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
      msg.m_ModelTransforms = ezArrayPtr<const ezMat4>(reinterpret_cast<const ezMat4*>(&modelTransforms[0]), (ezUInt32)modelTransforms.size());

      OnAnimationPoseUpdated(msg);
    }
  }

  TriggerLocalBoundsUpdate();
}

//void ezVisualizeSkeletonComponent::CreateSkeletonGeometry(const ezSkeleton* pSkeletonData, ezGeometry& geo)
//{
//  const ezUInt32 uiNumJoints = pSkeletonData->GetJointCount();
//
//  for (ezUInt32 b = 0; b < uiNumJoints; ++b)
//  {
//    const auto& joint = pSkeletonData->GetJointByIndex(b);
//
//    const ezTransform mJoint = ComputeJointTransform(*pSkeletonData, joint);
//
//    geo.AddSphere(0.03f, 10, 10, ezColor::RebeccaPurple, mJoint.GetAsMat4(), b);
//
//    if (!joint.IsRootJoint())
//    {
//      const ezTransform mParentJoint = ComputeJointTransform(*pSkeletonData, pSkeletonData->GetJointByIndex(joint.GetParentIndex()));
//
//      const ezVec3 vTargetPos = mJoint.m_vPosition;
//      const ezVec3 vSourcePos = mParentJoint.m_vPosition;
//
//      ezVec3 vJointDir = vTargetPos - vSourcePos;
//      const float fJointLen = vJointDir.GetLength();
//
//      if (fJointLen <= 0.0f)
//        continue;
//
//      vJointDir /= fJointLen;
//
//      ezMat4 mScale;
//      mScale.SetScalingMatrix(ezVec3(1, 1, fJointLen));
//
//      ezQuat qRot;
//      qRot.SetShortestRotation(ezVec3(0, 0, 1), vJointDir);
//
//      ezMat4 mTransform;
//      mTransform = qRot.GetAsMat4() * mScale;
//      mTransform.SetTranslationVector(vSourcePos);
//
//      geo.AddCone(0.02f, 1.0f, false, 4, ezColor::CornflowerBlue /* The Original! */, mTransform, b);
//    }
//  }
//}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_VisualizeSkeletonComponent);
