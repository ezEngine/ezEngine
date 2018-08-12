#include <PCH.h>

#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodSoundBankResource, 1, ezRTTIDefaultAllocator<ezFmodSoundBankResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezFmodSoundBankResource::ezFmodSoundBankResource()
    : ezResource<ezFmodSoundBankResource, ezFmodSoundBankResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezFmodSoundBankResource);
}

ezFmodSoundBankResource::~ezFmodSoundBankResource()
{
  EZ_ASSERT_DEV(m_pSoundBank == nullptr, "Soundbank has not been freed correctly");
}

ezResourceLoadDesc ezFmodSoundBankResource::UnloadData(Unload WhatToUnload)
{
  if (m_pSoundBank)
  {
    m_pSoundBank->unload();
    m_pSoundBank = nullptr;
  }

  ezFmod::GetSingleton()->QueueSoundBankDataForDeletion(m_pSoundBankData);
  m_pSoundBankData = nullptr;

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezFmodSoundBankResource);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 1;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezFmodSoundBankResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezFmodSoundBankResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  Stream->ReadBytes(&m_pSoundBank, sizeof(FMOD::Studio::Bank*));
  Stream->ReadBytes(&m_pSoundBankData, sizeof(ezDataBuffer*));

  EZ_ASSERT_DEV(m_pSoundBank != nullptr, "Invalid Sound Bank pointer in stream");
  EZ_ASSERT_DEV(m_pSoundBankData != nullptr, "Invalid Sound Bank Data pointer in stream");

  res.m_State = ezResourceState::Loaded;

  // the newly loaded sound bank might contain VCAs that had not been loaded yet
  ezFmod::GetSingleton()->UpdateSoundGroupVolumes();

  return res;
}

void ezFmodSoundBankResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezFmodSoundBankResource);

  if (m_pSoundBankData)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += (ezUInt32)m_pSoundBankData->GetHeapMemoryUsage() + sizeof(*m_pSoundBankData);
  }

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezFmodSoundBankResource::CreateResource(const ezFmodSoundBankResourceDescriptor& descriptor)
{
  // have to create one 'missing' resource
  // EZ_REPORT_FAILURE("This resource type does not support creating data.");

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}



EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_Resources_FmodSoundBankResource);
