#include <FmodPlugin/PCH.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <FmodPlugin/FmodSingleton.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodSoundBankResource, 1, ezRTTIDefaultAllocator<ezFmodSoundBankResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezFmodSoundBankResource::ezFmodSoundBankResource() : ezResource<ezFmodSoundBankResource, ezFmodSoundBankResourceDescriptor>(DoUpdate::OnMainThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezFmodSoundBankResource);

  m_pSoundBank = nullptr;
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

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezFmodSoundBankResource);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
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

  EZ_ASSERT_DEV(m_pSoundBank != nullptr, "Invalid Sound Bank pointer in stream");

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezFmodSoundBankResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = ModifyMemoryUsage().m_uiMemoryCPU;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezFmodSoundBankResource::CreateResource(const ezFmodSoundBankResourceDescriptor& descriptor)
{
  EZ_REPORT_FAILURE("This resource type does not support creating data.");

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}


