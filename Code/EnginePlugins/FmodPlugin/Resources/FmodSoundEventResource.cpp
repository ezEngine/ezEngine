#include <FmodPlugin/FmodPluginPCH.h>

#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodSoundEventResource, 1, ezRTTIDefaultAllocator<ezFmodSoundEventResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezFmodSoundEventResource);

ezFmodSoundEventResource::ezFmodSoundEventResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezFmodSoundEventResource);
}

ezFmodSoundEventResource::~ezFmodSoundEventResource()
{
  EZ_ASSERT_DEV(m_pEventDescription == nullptr, "SoundEvent has not been freed correctly");
}

ezResult ezFmodSoundEventResource::PlayOnce(const ezTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/) const
{
  bool bIsOneShot = false;
  m_pEventDescription->isOneshot(&bIsOneShot);

  if (!bIsOneShot)
  {
    ezLog::Warning("ezFmodSoundEventResource::PlayOnce: '{}' is not a one-shot event.", GetResourceIdOrDescription());
    return EZ_FAILURE;
  }

  auto pInstance = CreateInstance();
  if (pInstance == nullptr)
  {
    ezLog::Warning("ezFmodSoundEventResource::PlayOnce: Instance of '{}' could not be created.", GetResourceIdOrDescription());
    return EZ_FAILURE;
  }
  const auto fwd = globalPosition.m_qRotation * ezVec3(1, 0, 0);
  const auto up = globalPosition.m_qRotation * ezVec3(0, 0, 1);

  FMOD_3D_ATTRIBUTES attr;
  attr.position.x = globalPosition.m_vPosition.x;
  attr.position.y = globalPosition.m_vPosition.y;
  attr.position.z = globalPosition.m_vPosition.z;
  attr.forward.x = fwd.x;
  attr.forward.y = fwd.y;
  attr.forward.z = fwd.z;
  attr.up.x = up.x;
  attr.up.y = up.y;
  attr.up.z = up.z;
  attr.velocity.x = 0;
  attr.velocity.y = 0;
  attr.velocity.z = 0;

  EZ_FMOD_ASSERT(pInstance->setPitch(fPitch));
  EZ_FMOD_ASSERT(pInstance->setVolume(fVolume));
  EZ_FMOD_ASSERT(pInstance->set3DAttributes(&attr));
  EZ_FMOD_ASSERT(pInstance->setPaused(false));
  EZ_FMOD_ASSERT(pInstance->start());

  pInstance->release();

  return EZ_SUCCESS;
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
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezFmodSoundEventResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezFmodSoundEventResource::UpdateContent", GetResourceIdOrDescription());

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

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezFmodSoundEventResource, ezFmodSoundEventResourceDescriptor)
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
