#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipResource, 1, ezRTTIDefaultAllocator<ezAnimationClipResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


ezAnimationClipResource::ezAnimationClipResource()
    : ezResource<ezAnimationClipResource, ezAnimationClipResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezAnimationClipResource::CreateResource(const ezAnimationClipResourceDescriptor& descriptor)
{
  m_Descriptor = descriptor;

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
  res.m_uiQualityLevelsLoadable = 1;
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

  m_Descriptor.Load(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezAnimationClipResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezAnimationClipResource) + static_cast<ezUInt32>(m_Descriptor.GetHeapMemoryUsage());
}

void ezAnimationClipResourceDescriptor::Configure(ezUInt16 uiNumJoints, ezUInt16 uiNumFrames, ezUInt8 uiFramesPerSecond,
                                                  bool bIncludeRootMotion)
{
  EZ_ASSERT_DEV(uiNumFrames >= 2, "Invalid number of key frames");
  EZ_ASSERT_DEV(uiNumJoints > 0, "Invalid number of joints");
  EZ_ASSERT_DEV(uiFramesPerSecond > 0, "Invalid number of frames per second");

  m_uiNumJoints = uiNumJoints;
  m_uiNumFrames = uiNumFrames;
  m_uiFramesPerSecond = uiFramesPerSecond;

  m_Duration = ezTime::Seconds((double)(m_uiNumFrames-1) / (double)m_uiFramesPerSecond);

  ezUInt32 uiNumTransforms = m_uiNumJoints * m_uiNumFrames;

  if (bIncludeRootMotion)
  {
    uiNumTransforms += m_uiNumFrames;
    ezHashedString name;
    name.Assign("ezRootMotionTransform");
    AddJointName(name);
  }

  m_JointTransforms.SetCount(uiNumTransforms);
}

ezUInt16 ezAnimationClipResourceDescriptor::GetFrameAt(ezTime time, double& out_fLerpToNext) const
{
  const double fFrameIdx = time.GetSeconds() * m_uiFramesPerSecond;

  const ezUInt16 uiLowerFrame = static_cast<ezUInt16>(ezMath::Trunc(fFrameIdx));

  if (uiLowerFrame + 1 >= m_uiNumFrames)
  {
    out_fLerpToNext = 1;
    return m_uiNumFrames - 2;
  }

  out_fLerpToNext = ezMath::Fraction(fFrameIdx);

  return uiLowerFrame;
}

ezUInt16 ezAnimationClipResourceDescriptor::AddJointName(const ezHashedString& sJointName)
{
  const ezUInt16 uiJointIdx = m_JointNameToIndex.GetCount();
  m_JointNameToIndex.Insert(sJointName, uiJointIdx);
  return uiJointIdx;
}

ezUInt16 ezAnimationClipResourceDescriptor::FindJointIndexByName(const ezTempHashedString& sJointName) const
{
  const ezUInt32 uiIndex = m_JointNameToIndex.Find(sJointName);

  if (uiIndex == ezInvalidIndex)
    return ezInvalidJointIndex;

  return m_JointNameToIndex.GetValue(uiIndex);
}

ezArrayPtr<const ezTransform> ezAnimationClipResourceDescriptor::GetJointKeyframes(ezUInt16 uiJoint) const
{
  return ezArrayPtr<const ezTransform>(&m_JointTransforms[uiJoint * m_uiNumFrames], m_uiNumFrames);
}

ezArrayPtr<ezTransform> ezAnimationClipResourceDescriptor::GetJointKeyframes(ezUInt16 uiJoint)
{
  return ezArrayPtr<ezTransform>(&m_JointTransforms[uiJoint * m_uiNumFrames], m_uiNumFrames);
}

void ezAnimationClipResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 2;
  stream << uiVersion;

  stream << m_uiNumJoints;
  stream << m_uiNumFrames;
  stream << m_uiFramesPerSecond;

  stream << m_JointTransforms;

  // version 2
  {
    m_JointNameToIndex.Sort();
    const ezUInt32 uiJointCount = m_JointNameToIndex.GetCount();
    stream << uiJointCount;
    for (ezUInt32 b = 0; b < uiJointCount; ++b)
    {
      stream << m_JointNameToIndex.GetKey(b);
      stream << m_JointNameToIndex.GetValue(b);
    }
  }
}

void ezAnimationClipResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_uiNumJoints;
  stream >> m_uiNumFrames;
  stream >> m_uiFramesPerSecond;

  stream >> m_JointTransforms;

  m_Duration = ezTime::Seconds((double)(m_uiNumFrames-1) / (double)m_uiFramesPerSecond);

  // version 2
  if (uiVersion >= 2)
  {
    m_JointNameToIndex.Clear();
    ezUInt32 uiJointCount = 0;
    stream >> uiJointCount;

    for (ezUInt32 b = 0; b < uiJointCount; ++b)
    {
      ezHashedString hs;
      ezUInt16 idx;

      stream >> hs;
      stream >> idx;

      m_JointNameToIndex.Insert(hs, idx);
    }

    // should do nothing
    m_JointNameToIndex.Sort();
  }
}


ezUInt64 ezAnimationClipResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_JointTransforms.GetHeapMemoryUsage();
}

bool ezAnimationClipResourceDescriptor::HasRootMotion() const
{
  return m_JointNameToIndex.Contains(ezTempHashedString("ezRootMotionTransform"));
}

ezUInt16 ezAnimationClipResourceDescriptor::GetRootMotionJoint() const
{
  ezUInt16 jointIdx = 0;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

  const ezUInt32 idx = m_JointNameToIndex.Find(ezTempHashedString("ezRootMotionTransform"));
  EZ_ASSERT_DEBUG(idx != ezInvalidIndex, "Animation Clip has no root motion transforms");

  jointIdx = m_JointNameToIndex.GetValue(idx);
  EZ_ASSERT_DEBUG(jointIdx == 0, "The root motion joint should always be at index 0");
#endif

  return jointIdx;
}

void ezAnimationClipResourceDescriptor::SetPoseToKeyframe(ezAnimationPose& pose, const ezSkeleton& skeleton, ezUInt16 uiKeyframe) const
{
  for (ezUInt32 b = 0; b < m_JointNameToIndex.GetCount(); ++b)
  {
    const ezHashedString& sJointName = m_JointNameToIndex.GetKey(b);
    const ezUInt32 uiAnimJointIdx = m_JointNameToIndex.GetValue(b);

    const ezUInt16 uiSkeletonJointIdx = skeleton.FindJointByName(sJointName);
    if (uiSkeletonJointIdx != ezInvalidJointIndex)
    {
      ezArrayPtr<const ezTransform> pTransforms = GetJointKeyframes(uiAnimJointIdx);

      pose.SetTransform(uiSkeletonJointIdx, pTransforms[uiKeyframe].GetAsMat4());
    }
  }
}


void ezAnimationClipResourceDescriptor::SetPoseToBlendedKeyframe(ezAnimationPose& pose, const ezSkeleton& skeleton, ezUInt16 uiKeyframe0,
                                                                 float fBlendToKeyframe1) const
{
  for (ezUInt32 b = 0; b < m_JointNameToIndex.GetCount(); ++b)
  {
    const ezHashedString& sJointName = m_JointNameToIndex.GetKey(b);
    const ezUInt32 uiAnimJointIdx = m_JointNameToIndex.GetValue(b);

    const ezUInt16 uiSkeletonJointIdx = skeleton.FindJointByName(sJointName);
    if (uiSkeletonJointIdx != ezInvalidJointIndex)
    {
      ezArrayPtr<const ezTransform> pTransforms = GetJointKeyframes(uiAnimJointIdx);
      const ezTransform jointTransform1 = pTransforms[uiKeyframe0];
      const ezTransform jointTransform2 = pTransforms[uiKeyframe0 + 1];

      ezTransform res;
      res.m_vPosition = ezMath::Lerp(jointTransform1.m_vPosition, jointTransform2.m_vPosition, fBlendToKeyframe1);
      res.m_qRotation.SetSlerp(jointTransform1.m_qRotation, jointTransform2.m_qRotation, fBlendToKeyframe1);
      res.m_vScale = ezMath::Lerp(jointTransform1.m_vScale, jointTransform2.m_vScale, fBlendToKeyframe1);

      pose.SetTransform(uiSkeletonJointIdx, res.GetAsMat4());
    }
  }
}

ezTime ezAnimationClipResourceDescriptor::GetDuration() const
{
  return m_Duration;
}
