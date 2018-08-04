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

void ezAnimationClipResourceDescriptor::Configure(ezUInt16 uiNumBones, ezUInt16 uiNumFrames, ezUInt8 uiFramesPerSecond)
{
  EZ_ASSERT_DEV(uiNumFrames > 0, "Invalid number of animation frames");
  EZ_ASSERT_DEV(uiNumBones > 0, "Invalid number of animation bones");
  EZ_ASSERT_DEV(uiFramesPerSecond > 0, "Invalid number of animation fps");

  m_uiNumBones = uiNumBones;
  m_uiNumFrames = uiNumFrames;
  m_uiFramesPerSecond = uiFramesPerSecond;

  m_Duration = ezTime::Seconds((double)m_uiNumFrames / (double)m_uiFramesPerSecond);

  m_BoneTransforms.SetCount(m_uiNumBones * m_uiNumFrames);
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

const ezMat4* ezAnimationClipResourceDescriptor::GetFirstBones(ezUInt16 uiFrame) const
{
  return &m_BoneTransforms[uiFrame * m_uiNumBones];
}

ezMat4* ezAnimationClipResourceDescriptor::GetFirstBones(ezUInt16 uiFrame)
{
  return &m_BoneTransforms[uiFrame * m_uiNumBones];
}

void ezAnimationClipResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_uiNumBones;
  stream << m_uiNumFrames;
  stream << m_uiFramesPerSecond;

  stream << m_BoneTransforms;
}

void ezAnimationClipResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_uiNumBones;
  stream >> m_uiNumFrames;
  stream >> m_uiFramesPerSecond;

  stream >> m_BoneTransforms;

  m_Duration = ezTime::Seconds((double)m_uiNumFrames / (double)m_uiFramesPerSecond);
}


ezUInt64 ezAnimationClipResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_BoneTransforms.GetHeapMemoryUsage();
}

ezTime ezAnimationClipResourceDescriptor::GetDuration() const
{
  return m_Duration;
}
