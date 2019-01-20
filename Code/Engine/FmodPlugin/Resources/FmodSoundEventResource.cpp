#include <PCH.h>

#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodSoundEventResource, 1, ezRTTIDefaultAllocator<ezFmodSoundEventResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezFmodSoundEventResource::ezFmodSoundEventResource()
    : ezResource<ezFmodSoundEventResource, ezFmodSoundEventResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezFmodSoundEventResource);
}

ezFmodSoundEventResource::~ezFmodSoundEventResource()
{
  EZ_ASSERT_DEV(m_pEventDescription == nullptr, "SoundEvent has not been freed correctly");
}

FMOD::Studio::EventInstance* ezFmodSoundEventResource::CreateInstance() const
{
  if (m_pEventDescription)
  {
    FMOD::Studio::EventInstance* pInstance = nullptr;
    EZ_FMOD_ASSERT(m_pEventDescription->createInstance(&pInstance));
    return pInstance;
  }

  return nullptr;
}

ezResourceLoadDesc ezFmodSoundEventResource::UnloadData(Unload WhatToUnload)
{
  if (m_pEventDescription)
  {
    // this will kill all event pointers in the components
    // which is why the components actually listen for unload events on these resources
    m_pEventDescription->releaseAllInstances();
    m_pEventDescription->unloadSampleData();
    m_pEventDescription = nullptr;
  }

  m_hSoundBank.Invalidate();

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezFmodSoundEventResource);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 1;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezFmodSoundEventResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezFmodSoundEventResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  ezFmodSoundBankResourceHandle* pBankHandle = nullptr;
  Stream->ReadBytes(&pBankHandle, sizeof(ezFmodSoundBankResourceHandle*));
  EZ_ASSERT_DEV(pBankHandle != nullptr, "Invalid Sound Bank Handle pointer in stream");

  Stream->ReadBytes(&m_pEventDescription, sizeof(FMOD::Studio::EventDescription*));
  EZ_ASSERT_DEV(m_pEventDescription != nullptr, "Invalid Sound Event Descriptor pointer in stream");

  m_hSoundBank = *pBankHandle;

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezFmodSoundEventResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = ModifyMemoryUsage().m_uiMemoryCPU;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezFmodSoundEventResource::CreateResource(ezFmodSoundEventResourceDescriptor&& descriptor)
{
  // one missing resource is created this way
  // EZ_REPORT_FAILURE("This resource type does not support creating data.");

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}



EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_Resources_FmodSoundEventResource);
