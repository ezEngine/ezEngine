#include <RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/animation_optimizer.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipResource, 1, ezRTTIDefaultAllocator<ezAnimationClipResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezAnimationClipResource);
// clang-format on

ezAnimationClipResource::ezAnimationClipResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezAnimationClipResource, ezAnimationClipResourceDescriptor)
{
  m_Descriptor = std::move(descriptor);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

ezResourceLoadDesc ezAnimationClipResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezAnimationClipResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezAnimationClipResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  m_Descriptor.Deserialize(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezAnimationClipResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezAnimationClipResource) + static_cast<ezUInt32>(m_Descriptor.GetHeapMemoryUsage());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct ezAnimationClipResourceDescriptor::Impl
{
  ezUInt16 m_uiNumJoints = 0;
  ezUInt16 m_uiNumFrames = 0;
  ezTime m_Duration;

  ezArrayMap<ezHashedString, ezUInt16> m_JointNameToIndex;
  ezDynamicArray<ezTransform> m_JointTransforms;

  ezMap<const ezSkeletonResource*, ozz::unique_ptr<ozz::animation::Animation>> m_MappedOzzAnimations;
};

ezAnimationClipResourceDescriptor::ezAnimationClipResourceDescriptor()
{
  m_Impl = EZ_DEFAULT_NEW(Impl);
}

ezAnimationClipResourceDescriptor::ezAnimationClipResourceDescriptor(ezAnimationClipResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

ezAnimationClipResourceDescriptor::~ezAnimationClipResourceDescriptor() = default;

void ezAnimationClipResourceDescriptor::operator=(ezAnimationClipResourceDescriptor&& rhs) noexcept
{
  m_Impl = std::move(rhs.m_Impl);
}

void ezAnimationClipResourceDescriptor::Configure(ezUInt16 uiNumJoints, ezUInt16 uiNumFrames, ezTime duration /*, bool bIncludeRootMotion*/)
{
  EZ_ASSERT_DEV(uiNumFrames >= 2, "Invalid number of key frames");
  EZ_ASSERT_DEV(uiNumJoints > 0, "Invalid number of joints");
  EZ_ASSERT_DEV(duration > ezTime::Zero(), "Invalid duration");

  m_Impl->m_uiNumJoints = uiNumJoints;
  m_Impl->m_uiNumFrames = uiNumFrames;
  m_Impl->m_Duration = duration;

  //if (bIncludeRootMotion)
  //{
  //  ezHashedString name;
  //  name.Assign("ezRootMotionTransform");
  //  AddJointName(name);
  //}

  m_Impl->m_JointTransforms.SetCountUninitialized(m_Impl->m_uiNumJoints * m_Impl->m_uiNumFrames);
}

ezUInt16 ezAnimationClipResourceDescriptor::AddJointName(const ezHashedString& sJointName)
{
  const ezUInt16 uiJointIdx = m_Impl->m_JointNameToIndex.GetCount();
  m_Impl->m_JointNameToIndex.Insert(sJointName, uiJointIdx);
  return uiJointIdx;
}

ezUInt16 ezAnimationClipResourceDescriptor::FindJointIndexByName(const ezTempHashedString& sJointName) const
{
  const ezUInt32 uiIndex = m_Impl->m_JointNameToIndex.Find(sJointName);

  if (uiIndex == ezInvalidIndex)
    return ezInvalidJointIndex;

  return m_Impl->m_JointNameToIndex.GetValue(uiIndex);
}

const ezArrayMap<ezHashedString, ezUInt16>& ezAnimationClipResourceDescriptor::GetAllJointIndices() const
{
  return m_Impl->m_JointNameToIndex;
}

ezResult ezAnimationClipResourceDescriptor::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(5);

  stream << m_Impl->m_uiNumJoints;
  stream << m_Impl->m_uiNumFrames;
  stream << m_Impl->m_Duration;

  m_Impl->m_JointNameToIndex.Sort();
  const ezUInt32 uiJointCount = m_Impl->m_JointNameToIndex.GetCount();
  stream << uiJointCount;
  for (ezUInt32 b = 0; b < uiJointCount; ++b)
  {
    stream << m_Impl->m_JointNameToIndex.GetKey(b);
    stream << m_Impl->m_JointNameToIndex.GetValue(b);
  }

  stream.WriteArray(m_Impl->m_JointTransforms);

  return EZ_SUCCESS;
}

ezResult ezAnimationClipResourceDescriptor::Deserialize(ezStreamReader& stream)
{
  const ezTypeVersion uiVersion = stream.ReadVersion(5);
  EZ_ASSERT_ALWAYS(uiVersion >= 5, "Unsupported version");

  stream >> m_Impl->m_uiNumJoints;
  stream >> m_Impl->m_uiNumFrames;
  stream >> m_Impl->m_Duration;

  m_Impl->m_JointNameToIndex.Clear();
  ezUInt32 uiJointCount = 0;
  stream >> uiJointCount;

  for (ezUInt32 b = 0; b < uiJointCount; ++b)
  {
    ezHashedString hs;
    ezUInt16 idx;

    stream >> hs;
    stream >> idx;

    m_Impl->m_JointNameToIndex.Insert(hs, idx);
  }

  // should do nothing
  m_Impl->m_JointNameToIndex.Sort();

  stream.ReadArray(m_Impl->m_JointTransforms);

  return EZ_SUCCESS;
}

ezUInt64 ezAnimationClipResourceDescriptor::GetHeapMemoryUsage() const
{
  return /*(ezUInt64)m_pOzzData->size() +*/ m_Impl->m_JointNameToIndex.GetHeapMemoryUsage() + m_Impl->m_JointTransforms.GetHeapMemoryUsage();
}

ezUInt16 ezAnimationClipResourceDescriptor::GetNumJoints() const
{
  return m_Impl->m_uiNumJoints;
}

ezUInt16 ezAnimationClipResourceDescriptor::GetNumFrames() const
{
  return m_Impl->m_uiNumFrames;
}

ezTime ezAnimationClipResourceDescriptor::GetDuration() const
{
  return m_Impl->m_Duration;
}

ezArrayPtr<const ezTransform> ezAnimationClipResourceDescriptor::GetJointKeyframes(ezUInt16 uiJoint) const
{
  return ezArrayPtr<const ezTransform>(&m_Impl->m_JointTransforms[uiJoint * m_Impl->m_uiNumFrames], m_Impl->m_uiNumFrames);
}

ezArrayPtr<ezTransform> ezAnimationClipResourceDescriptor::GetJointKeyframes(ezUInt16 uiJoint)
{
  return ezArrayPtr<ezTransform>(&m_Impl->m_JointTransforms[uiJoint * m_Impl->m_uiNumFrames], m_Impl->m_uiNumFrames);
}

const ozz::animation::Animation& ezAnimationClipResourceDescriptor::GetMappedOzzAnimation(const ezSkeletonResource& skeleton) const
{
  auto it = m_Impl->m_MappedOzzAnimations.Find(&skeleton);
  if (it.IsValid())
    return *it.Value().get();

  auto pOzzSkeleton = &skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton();
  const ezUInt32 uiNumJoints = pOzzSkeleton->num_joints();

  ozz::animation::offline::RawAnimation rawAnim;
  rawAnim.duration = m_Impl->m_Duration.AsFloatInSeconds();
  rawAnim.tracks.resize(uiNumJoints);

  const double fStepTime = m_Impl->m_Duration.GetSeconds() / m_Impl->m_uiNumFrames;

  for (ezUInt32 j = 0; j < uiNumJoints; ++j)
  {
    auto& dstTrack = rawAnim.tracks[j];
    dstTrack.translations.resize(m_Impl->m_uiNumFrames);
    dstTrack.rotations.resize(m_Impl->m_uiNumFrames);
    dstTrack.scales.resize(m_Impl->m_uiNumFrames);

    const ezTempHashedString sJointName = ezTempHashedString(pOzzSkeleton->joint_names()[j]);

    const ezUInt32 uiSrcIdx = FindJointIndexByName(sJointName);

    if (uiSrcIdx == ezInvalidJointIndex)
    {
      const ezUInt16 uiFallbackIdx = skeleton.GetDescriptor().m_Skeleton.FindJointByName(sJointName);

      EZ_ASSERT_DEV(uiFallbackIdx != ezInvalidJointIndex, "");

      const auto& fallbackJoint = skeleton.GetDescriptor().m_Skeleton.GetJointByIndex(uiFallbackIdx);

      const ezTransform& fallbackTransform = fallbackJoint.GetBindPoseLocalTransform();

      for (ezUInt32 f = 0; f < m_Impl->m_uiNumFrames; ++f)
      {
        auto& dstT = dstTrack.translations[f];
        auto& dstR = dstTrack.rotations[f];
        auto& dstS = dstTrack.scales[f];

        dstT.time = (float)(fStepTime * f);
        dstR.time = dstT.time;
        dstS.time = dstT.time;

        dstT.value.x = fallbackTransform.m_vPosition.x;
        dstT.value.y = fallbackTransform.m_vPosition.y;
        dstT.value.z = fallbackTransform.m_vPosition.z;

        dstR.value.x = fallbackTransform.m_qRotation.v.x;
        dstR.value.y = fallbackTransform.m_qRotation.v.y;
        dstR.value.z = fallbackTransform.m_qRotation.v.z;
        dstR.value.w = fallbackTransform.m_qRotation.w;

        dstS.value.x = fallbackTransform.m_vScale.x;
        dstS.value.y = fallbackTransform.m_vScale.y;
        dstS.value.z = fallbackTransform.m_vScale.z;
      }
    }
    else
    {
      const auto& srcTrack = GetJointKeyframes(uiSrcIdx);

      for (ezUInt32 f = 0; f < m_Impl->m_uiNumFrames; ++f)
      {
        const auto& srcJoint = srcTrack[f];

        auto& dstT = dstTrack.translations[f];
        auto& dstR = dstTrack.rotations[f];
        auto& dstS = dstTrack.scales[f];

        dstT.time = (float)(fStepTime * f);
        dstR.time = dstT.time;
        dstS.time = dstT.time;

        dstT.value.x = srcJoint.m_vPosition.x;
        dstT.value.y = srcJoint.m_vPosition.y;
        dstT.value.z = srcJoint.m_vPosition.z;

        dstR.value.x = srcJoint.m_qRotation.v.x;
        dstR.value.y = srcJoint.m_qRotation.v.y;
        dstR.value.z = srcJoint.m_qRotation.v.z;
        dstR.value.w = srcJoint.m_qRotation.w;

        dstS.value.x = srcJoint.m_vScale.x;
        dstS.value.y = srcJoint.m_vScale.y;
        dstS.value.z = srcJoint.m_vScale.z;
      }
    }
  }

  // TODO: optimize animation
  {
    //ozz::animation::offline::RawAnimation optAnim;

    //ozz::animation::offline::AnimationOptimizer optimizer;
    //if (!optimizer(rawAnim, *pOzzSkeleton, &optAnim))
    //  return ezStatus("Failed to optimize animation");
  }

  ozz::animation::offline::AnimationBuilder animBuilder;

  m_Impl->m_MappedOzzAnimations[&skeleton] = std::move(animBuilder(rawAnim));

  return *m_Impl->m_MappedOzzAnimations[&skeleton].get();
}

//ezUInt16 ezAnimationClipResourceDescriptor::GetFrameAt(ezTime time, double& out_fLerpToNext) const
//{
//  const double fFrameIdx = time.GetSeconds() * m_uiFramesPerSecond;
//
//  const ezUInt16 uiLowerFrame = static_cast<ezUInt16>(ezMath::Trunc(fFrameIdx));
//
//  if (uiLowerFrame + 1 >= m_uiNumFrames)
//  {
//    out_fLerpToNext = 1;
//    return m_uiNumFrames - 2;
//  }
//
//  out_fLerpToNext = ezMath::Fraction(fFrameIdx);
//
//  return uiLowerFrame;
//}


//bool ezAnimationClipResourceDescriptor::HasRootMotion() const
//{
//  return m_JointNameToIndex.Contains(ezTempHashedString("ezRootMotionTransform"));
//}
//
//ezUInt16 ezAnimationClipResourceDescriptor::GetRootMotionJoint() const
//{
//  ezUInt16 jointIdx = 0;
//
//#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
//
//  const ezUInt32 idx = m_JointNameToIndex.Find(ezTempHashedString("ezRootMotionTransform"));
//  EZ_ASSERT_DEBUG(idx != ezInvalidIndex, "Animation Clip has no root motion transforms");
//
//  jointIdx = m_JointNameToIndex.GetValue(idx);
//  EZ_ASSERT_DEBUG(jointIdx == 0, "The root motion joint should always be at index 0");
//#endif
//
//  return jointIdx;
//}
//
//void ezAnimationClipResourceDescriptor::SetPoseToKeyframe(ezAnimationPose& pose, const ezSkeleton& skeleton, ezUInt16 uiKeyframe) const
//{
//  for (ezUInt32 b = 0; b < m_JointNameToIndex.GetCount(); ++b)
//  {
//    const ezHashedString& sJointName = m_JointNameToIndex.GetKey(b);
//    const ezUInt32 uiAnimJointIdx = m_JointNameToIndex.GetValue(b);
//
//    const ezUInt16 uiSkeletonJointIdx = skeleton.FindJointByName(sJointName);
//    if (uiSkeletonJointIdx != ezInvalidJointIndex)
//    {
//      ezArrayPtr<const ezTransform> pTransforms = GetJointKeyframes(uiAnimJointIdx);
//
//      pose.SetTransform(uiSkeletonJointIdx, pTransforms[uiKeyframe].GetAsMat4());
//    }
//  }
//}
//
//
//void ezAnimationClipResourceDescriptor::SetPoseToBlendedKeyframe(ezAnimationPose& pose, const ezSkeleton& skeleton, ezUInt16 uiKeyframe0, float fBlendToKeyframe1) const
//{
//  for (ezUInt32 b = 0; b < m_JointNameToIndex.GetCount(); ++b)
//  {
//    const ezHashedString& sJointName = m_JointNameToIndex.GetKey(b);
//    const ezUInt32 uiAnimJointIdx = m_JointNameToIndex.GetValue(b);
//
//    const ezUInt16 uiSkeletonJointIdx = skeleton.FindJointByName(sJointName);
//    if (uiSkeletonJointIdx != ezInvalidJointIndex)
//    {
//      ezArrayPtr<const ezTransform> pTransforms = GetJointKeyframes(uiAnimJointIdx);
//      const ezTransform jointTransform1 = pTransforms[uiKeyframe0];
//      const ezTransform jointTransform2 = pTransforms[uiKeyframe0 + 1];
//
//      ezTransform res;
//      res.m_vPosition = ezMath::Lerp(jointTransform1.m_vPosition, jointTransform2.m_vPosition, fBlendToKeyframe1);
//      res.m_qRotation.SetSlerp(jointTransform1.m_qRotation, jointTransform2.m_qRotation, fBlendToKeyframe1);
//      res.m_vScale = ezMath::Lerp(jointTransform1.m_vScale, jointTransform2.m_vScale, fBlendToKeyframe1);
//
//      pose.SetTransform(uiSkeletonJointIdx, res.GetAsMat4());
//    }
//  }
//}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationClipResource);
