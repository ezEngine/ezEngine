#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

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

void ezAnimationClipResourceDescriptor::Configure(ezUInt16 uiNumJoints, ezUInt16 uiNumFrames, ezUInt8 uiFramesPerSecond)
{
  EZ_ASSERT_DEV(uiNumFrames > 0, "Invalid number of animation frames");
  EZ_ASSERT_DEV(uiNumJoints > 0, "Invalid number of animation joints");
  EZ_ASSERT_DEV(uiFramesPerSecond > 0, "Invalid number of animation fps");

  m_uiNumJoints = uiNumJoints;
  m_uiNumFrames = uiNumFrames;
  m_uiFramesPerSecond = uiFramesPerSecond;

  m_Duration = ezTime::Seconds((double)m_uiNumFrames / (double)m_uiFramesPerSecond);

  m_JointTransforms.SetCount(m_uiNumJoints * m_uiNumFrames);
}

ezUInt16 ezAnimationClipResourceDescriptor::GetFrameAt(ezTime time, double& out_fLerpToNext) const
{
  const double fFrameIdx = time.GetSeconds() * m_uiFramesPerSecond;

  if (fFrameIdx >= m_uiNumFrames)
  {
    out_fLerpToNext = 0;
    return m_uiNumFrames - 1;
  }

  const ezUInt16 uiLowerFrame = static_cast<ezUInt16>(ezMath::Trunc(fFrameIdx));
  out_fLerpToNext = ezMath::Fraction(fFrameIdx);

  return uiLowerFrame;
}

ezUInt16 ezAnimationClipResourceDescriptor::AddJointName(const ezHashedString& sJointName)
{
  const ezUInt16 uiJointIdx = m_NameToFirstKeyframe.GetCount();
  m_NameToFirstKeyframe.Insert(sJointName, uiJointIdx);
  return uiJointIdx;
}

ezUInt16 ezAnimationClipResourceDescriptor::FindJointIndexByName(const ezTempHashedString& sJointName) const
{
  const ezUInt32 uiIndex = m_NameToFirstKeyframe.Find(sJointName);

  if (uiIndex == ezInvalidIndex)
    return 0xFFFF;

  return m_NameToFirstKeyframe.GetValue(uiIndex);
}

const ezTransform* ezAnimationClipResourceDescriptor::GetJointKeyframes(ezUInt16 uiJoint) const
{
  return &m_JointTransforms[uiJoint * m_uiNumFrames];
}

ezTransform* ezAnimationClipResourceDescriptor::GetJointKeyframes(ezUInt16 uiJoint)
{
  return &m_JointTransforms[uiJoint * m_uiNumFrames];
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
    m_NameToFirstKeyframe.Sort();
    const ezUInt32 uiJointCount = m_NameToFirstKeyframe.GetCount();
    stream << uiJointCount;
    for (ezUInt32 b = 0; b < uiJointCount; ++b)
    {
      stream << m_NameToFirstKeyframe.GetKey(b);
      stream << m_NameToFirstKeyframe.GetValue(b);
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

  m_Duration = ezTime::Seconds((double)m_uiNumFrames / (double)m_uiFramesPerSecond);

  // version 2
  if (uiVersion >= 2)
  {
    m_NameToFirstKeyframe.Clear();
    ezUInt32 uiJointCount = 0;
    stream >> uiJointCount;

    for (ezUInt32 b = 0; b < uiJointCount; ++b)
    {
      ezHashedString hs;
      ezUInt32 idx;

      stream >> hs;
      stream >> idx;

      m_NameToFirstKeyframe.Insert(hs, idx);
    }

    // should do nothing
    m_NameToFirstKeyframe.Sort();
  }
}


ezUInt64 ezAnimationClipResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_JointTransforms.GetHeapMemoryUsage();
}

ezTime ezAnimationClipResourceDescriptor::GetDuration() const
{
  return m_Duration;
}
