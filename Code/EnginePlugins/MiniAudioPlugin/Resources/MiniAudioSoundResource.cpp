#include <MiniAudioPlugin/MiniAudioPluginPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <MiniAudioPlugin/MiniAudioSingleton.h>
#include <MiniAudioPlugin/Resources/MiniAudioSoundResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMiniAudioSoundResource, 1, ezRTTIDefaultAllocator<ezMiniAudioSoundResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezMiniAudioSoundResource);

ezMiniAudioSoundResource::ezMiniAudioSoundResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezMiniAudioSoundResource);
}

ezMiniAudioSoundResource::~ezMiniAudioSoundResource() = default;

const ezDataBuffer& ezMiniAudioSoundResource::GetAudioData() const
{
  return m_AudioData[0];
}

const ezDataBuffer& ezMiniAudioSoundResource::GetAudioData(ezRandom& ref_rng) const
{
  return m_AudioData[ref_rng.UInt32Index(m_AudioData.GetCount())];
}

float ezMiniAudioSoundResource::GetVolume(ezRandom& ref_rng) const
{
  if (m_fMinVolume < m_fMaxVolume)
    return ref_rng.FloatMinMax(m_fMinVolume, m_fMaxVolume);

  return m_fMinVolume;
}

float ezMiniAudioSoundResource::GetPitch(ezRandom& ref_rng) const
{
  if (m_fMinPitch < m_fMaxPitch)
    return ref_rng.FloatMinMax(m_fMinPitch, m_fMaxPitch);

  return m_fMinPitch;
}

ezResourceLoadDesc ezMiniAudioSoundResource::UnloadData(Unload WhatToUnload)
{
  m_AudioData.Clear();
  m_AudioData.Compact();

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezMiniAudioSoundResource);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezMiniAudioSoundResource::UpdateContent(ezStreamReader* pStream)
{
  EZ_LOG_BLOCK("ezMiniAudioSoundResource::UpdateContent", GetResourceIdOrDescription());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (pStream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  ezString sAbsFilePath;
  (*pStream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  ezUInt8 uiVersion = 0;
  *pStream >> uiVersion;

  *pStream >> m_bLoop;
  *pStream >> m_fMinVolume;
  *pStream >> m_fMaxVolume;
  *pStream >> m_fMinPitch;
  *pStream >> m_fMaxPitch;
  *pStream >> m_bSpatialize;
  *pStream >> m_fMinDistance;
  *pStream >> m_fMaxDistance;
  *pStream >> m_fRolloff;
  *pStream >> m_fDopplerFactor;

  if (m_fMinVolume > m_fMaxVolume)
    ezMath::Swap(m_fMinVolume, m_fMaxVolume);

  if (m_fMinDistance > m_fMaxDistance)
    ezMath::Swap(m_fMinDistance, m_fMaxDistance);

  if (m_fMinPitch > m_fMaxPitch)
    ezMath::Swap(m_fMinPitch, m_fMaxPitch);

  ezUInt32 uiNumFiles = 0;
  *pStream >> uiNumFiles;

  m_AudioData.SetCount(uiNumFiles);

  for (ezUInt32 i = 0; i < uiNumFiles; ++i)
  {
    ezUInt32 uiFileSize = 0;
    *pStream >> uiFileSize;

    m_AudioData[i].SetCountUninitialized(uiFileSize);
    if (pStream->ReadBytes(m_AudioData[i].GetData(), uiFileSize) != uiFileSize)
    {
      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
    }
  }

  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezMiniAudioSoundResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezMiniAudioSoundResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_AudioData.GetHeapMemoryUsage();

  for (const auto& data : m_AudioData)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += data.GetHeapMemoryUsage();
  }

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezMiniAudioSoundResource, ezMiniAudioSoundResourceDescriptor)
{
  // have to create one 'missing' resource
  // EZ_REPORT_FAILURE("This resource type does not support creating data.");

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}
