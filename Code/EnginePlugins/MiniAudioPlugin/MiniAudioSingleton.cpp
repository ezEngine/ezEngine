#include <MiniAudioPlugin/MiniAudioPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <MiniAudioPlugin/Components/MiniAudioSoundComponent.h>
#include <MiniAudioPlugin/MiniAudioSingleton.h>
#include <MiniAudioPlugin/Resources/MiniAudioSoundResource.h>

ezCVarFloat cvar_MiniAudioMasterVolume("MiniAudio.Volume", 1.0f, ezCVarFlags::Save, "Overall volume for all MiniAudio output");
ezCVarBool cvar_MiniAudioMute("MiniAudio.Mute", false, ezCVarFlags::Default, "Whether MiniAudio output is muted");
ezCVarBool cvar_MiniAudioPause("MiniAudio.Pause", false, ezCVarFlags::Default, "Whether MiniAudio output is paused");

EZ_IMPLEMENT_SINGLETON(ezMiniAudioSingleton);

static ezMiniAudioSingleton g_MiniAudioSingleton;

ezMiniAudioSingleton::ezMiniAudioSingleton()
  : m_SingletonRegistrar(this)
{
  m_bInitialized = false;
  m_vListenerPosition.SetZero();
}

ezMiniAudioSingleton::~ezMiniAudioSingleton() = default;

void ezMiniAudioSingleton::Startup()
{
  if (m_bInitialized)
    return;

  m_pData = EZ_DEFAULT_NEW(Data);

  EZ_LOCK(m_pData->m_Mutex);

  ma_engine_config cfg = ma_engine_config_init();
  // cfg.pLog
  cfg.listenerCount = 1;
  cfg.channels = 0; // native channel count
  // cfg.allocationCallbacks
  // pContext

  auto result = ma_engine_init(&cfg, &m_pData->m_Engine);
  if (result != MA_SUCCESS)
    return;

  m_bInitialized = true;
  UpdateSound();
}

void ezMiniAudioSingleton::Shutdown()
{
  EZ_LOCK(m_pData->m_Mutex);

  if (m_bInitialized)
  {
    m_bInitialized = false;
    ma_engine_uninit(&m_pData->m_Engine);
  }

  // finally delete all data
  m_pData.Clear();
}

void ezMiniAudioSingleton::SetNumListeners(ezUInt8 uiNumListeners)
{
}

ezUInt8 ezMiniAudioSingleton::GetNumListeners()
{
  return 1;
}

void ezMiniAudioSingleton::LoadConfiguration(ezStringView sFile)
{
}

void ezMiniAudioSingleton::SetOverridePlatform(ezStringView sPlatform)
{
}

void ezMiniAudioSingleton::UpdateSound()
{
  if (!m_bInitialized)
    return;

  EZ_LOCK(m_pData->m_Mutex);

  if (cvar_MiniAudioPause)
  {
    EZ_MA_CHECK(ma_engine_stop(&m_pData->m_Engine));
  }
  else
  {
    EZ_MA_CHECK(ma_engine_start(&m_pData->m_Engine));

    if (cvar_MiniAudioMute)
    {
      EZ_MA_CHECK(ma_engine_set_volume(&m_pData->m_Engine, 0.0f));
    }
    else
    {
      EZ_MA_CHECK(ma_engine_set_volume(&m_pData->m_Engine, cvar_MiniAudioMasterVolume));
    }
  }

  if (!m_pData->m_FinishedInstances.IsEmpty())
  {
    for (ezUInt32 idx : m_pData->m_FinishedInstances)
    {
      ezMiniAudioSoundInstance* pInstance = &m_pData->m_SoundInstancesStorage[idx];

      if (!pInstance->m_hComponent.IsInvalidated())
      {
        ezMiniAudioSoundComponent* pSoundComp;

        EZ_LOCK(pInstance->pWorld->GetWriteMarker());

        if (pInstance->pWorld->TryGetComponent(pInstance->m_hComponent, pSoundComp))
        {
          if (pSoundComp->IsActiveAndSimulating())
          {
            pSoundComp->SoundFinished();
          }
        }
      }

      FreeSoundInstance(pInstance);
    }

    m_pData->m_FinishedInstances.Clear();
  }

  if (!m_pData->m_FadingInstances.IsEmpty())
  {
    for (ezUInt32 i = 0; i < m_pData->m_FadingInstances.GetCount(); ++i)
    {
      const ezUInt32 idx = m_pData->m_FadingInstances[i];
      ezMiniAudioSoundInstance* pInstance = &m_pData->m_SoundInstancesStorage[idx];

      if (!ma_sound_is_playing(&pInstance->m_Sound) || ma_sound_get_current_fade_volume(&pInstance->m_Sound) <= 0.0f)
      {
        FreeSoundInstance(pInstance);

        m_pData->m_FadingInstances.RemoveAtAndSwap(i);
        break;
      }
    }
  }
}

void ezMiniAudioSingleton::SetMasterChannelVolume(float fVolume)
{
  cvar_MiniAudioMasterVolume = ezMath::Clamp<float>(fVolume, 0.0f, 1.0f);
}

float ezMiniAudioSingleton::GetMasterChannelVolume() const
{
  return cvar_MiniAudioMasterVolume;
}

void ezMiniAudioSingleton::SetMasterChannelMute(bool bMute)
{
  cvar_MiniAudioMute = bMute;
}

bool ezMiniAudioSingleton::GetMasterChannelMute() const
{
  return cvar_MiniAudioMute;
}

void ezMiniAudioSingleton::SetMasterChannelPaused(bool bPaused)
{
  cvar_MiniAudioPause = bPaused;
}

bool ezMiniAudioSingleton::GetMasterChannelPaused() const
{
  return cvar_MiniAudioPause;
}

void ezMiniAudioSingleton::SetSoundGroupVolume(ezStringView sVcaGroupGuid, float fVolume)
{
  UpdateSoundGroupVolumes();
}

float ezMiniAudioSingleton::GetSoundGroupVolume(ezStringView sVcaGroupGuid) const
{
  return 1.0f;
}

void ezMiniAudioSingleton::UpdateSoundGroupVolumes()
{
}

void ezMiniAudioSingleton::GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e)
{
  if (e.m_Type == ezGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    ezMiniAudioSingleton::GetSingleton()->UpdateSound();
  }
}

void ezMiniAudioSingleton::SetListenerOverrideMode(bool bEnabled)
{
  m_bListenerOverrideMode = bEnabled;
}

void ezMiniAudioSingleton::SetListener(ezInt32 iIndex, const ezVec3& vPosition, const ezVec3& vForward, const ezVec3& vUp, const ezVec3& vVelocity)
{
  if (m_bListenerOverrideMode)
  {
    if (iIndex != -1)
      return;

    iIndex = 0;
  }

  if (iIndex < 0 || iIndex >= /*FMOD_MAX_LISTENERS*/ 1)
    return;

  if (iIndex == 0)
  {
    m_vListenerPosition = vPosition;
  }

  const ezVec3 vMaUp = -vUp; // EZ and MiniAudio use different coordinate systems

  ma_engine_listener_set_position(&m_pData->m_Engine, iIndex, vPosition.x, vPosition.y, vPosition.z);
  ma_engine_listener_set_direction(&m_pData->m_Engine, iIndex, vForward.x, vForward.y, vForward.z);
  ma_engine_listener_set_world_up(&m_pData->m_Engine, iIndex, vMaUp.x, vMaUp.y, vMaUp.z);
  ma_engine_listener_set_velocity(&m_pData->m_Engine, iIndex, vVelocity.x, vVelocity.y, vVelocity.z);
}

ezResult ezMiniAudioSingleton::OneShotSound(ezStringView sResourceID, const ezTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  ezMiniAudioSoundResourceHandle hSound = ezResourceManager::LoadResource<ezMiniAudioSoundResource>(sResourceID);

  if (!hSound.IsValid())
    return EZ_FAILURE;

  ezResourceLock<ezMiniAudioSoundResource> pResource(hSound, bBlockIfNotLoaded ? ezResourceAcquireMode::BlockTillLoaded_NeverFail : ezResourceAcquireMode::AllowLoadingFallback_NeverFail);

  if (pResource.GetAcquireResult() != ezResourceAcquireResult::Final)
    return EZ_FAILURE;

  if (pResource->GetLoop())
    return EZ_FAILURE; // never play looping sounds

  // can't randomize anything here, we have no world and therefore no random number generator

  auto pInstance = AllocateSoundInstance(pResource->GetAudioData(), nullptr, {});

  ma_sound_set_min_distance(&pInstance->m_Sound, pResource->GetMinDistance());
  ma_sound_set_rolloff(&pInstance->m_Sound, pResource->GetRolloff());
  ma_sound_set_doppler_factor(&pInstance->m_Sound, pResource->GetDopplerFactor());
  ma_sound_set_spatialization_enabled(&pInstance->m_Sound, pResource->GetSpatialize());

  const ezVec3 pos = globalPosition.m_vPosition;
  ma_sound_set_position(&pInstance->m_Sound, pos.x, pos.y, pos.z);
  ma_sound_set_pitch(&pInstance->m_Sound, fPitch);
  ma_sound_set_volume(&pInstance->m_Sound, fVolume);

  // the sound will play till its end and then get cleaned up automatically
  EZ_MA_CHECK(ma_sound_start(&pInstance->m_Sound));

  return EZ_SUCCESS;
}

static void SoundEndedCallback(void* pUserData, ma_sound* pSound)
{
  ezMiniAudioSingleton::GetSingleton()->SoundEnded((ezMiniAudioSoundInstance*)pUserData);
}

ezMiniAudioSoundInstance* ezMiniAudioSingleton::AllocateSoundInstance(const ezDataBuffer& audioData, ezWorld* pWorld, ezComponentHandle hComponent)
{
  if (!m_bInitialized)
    return nullptr;

  EZ_LOCK(m_pData->m_Mutex);

  ezMiniAudioSoundInstance* pInstance = nullptr;

  if (!m_pData->m_SoundInstanceFreeList.IsEmpty())
  {
    const ezUInt32 idx = m_pData->m_SoundInstanceFreeList.PeekBack();
    m_pData->m_SoundInstanceFreeList.PopBack();
    pInstance = &m_pData->m_SoundInstancesStorage[idx];
  }
  else
  {
    const ezUInt32 idx = m_pData->m_SoundInstancesStorage.GetCount();
    pInstance = &m_pData->m_SoundInstancesStorage.ExpandAndGetRef();
    pInstance->m_uiOwnIndex = idx;
  }

  pInstance->m_bInUse = true;
  pInstance->pWorld = pWorld;
  pInstance->m_hComponent = hComponent;

  if (ma_decoder_init_memory(audioData.GetData(), audioData.GetCount(), nullptr, &pInstance->m_Decoder) != MA_SUCCESS)
  {
    FreeSoundInstance(pInstance);
    return nullptr;
  }

  if (ma_sound_init_from_data_source(GetEngine(), &pInstance->m_Decoder, MA_SOUND_FLAG_DECODE, nullptr, &pInstance->m_Sound) != MA_SUCCESS)
  {
    FreeSoundInstance(pInstance);
    return nullptr;
  }

  // make sure to be notified when the sound ends
  EZ_MA_CHECK(ma_sound_set_end_callback(&pInstance->m_Sound, SoundEndedCallback, pInstance));

  return pInstance;
}

void ezMiniAudioSingleton::FreeSoundInstance(ezMiniAudioSoundInstance*& ref_pInstance)
{
  const ezUInt32 uiIndex = ref_pInstance->m_uiOwnIndex;

  if (!ref_pInstance->m_bInUse) // already freed
  {
    EZ_ASSERT_DEBUG(m_pData->m_SoundInstanceFreeList.Contains(uiIndex), "Sound is not in the free list");
    return;
  }

  EZ_LOCK(m_pData->m_Mutex);

  EZ_ASSERT_DEBUG(!m_pData->m_SoundInstanceFreeList.Contains(uiIndex), "Sound is already freed");

  ref_pInstance->m_bInUse = false;

  auto& inst = m_pData->m_SoundInstancesStorage[uiIndex];
  inst.m_hComponent.Invalidate();
  inst.pWorld = nullptr;

  EZ_MA_CHECK(ma_sound_stop(&inst.m_Sound));
  EZ_MA_CHECK(ma_decoder_uninit(&inst.m_Decoder));
  ma_sound_uninit(&inst.m_Sound);

  m_pData->m_SoundInstanceFreeList.PushBack(uiIndex);

  ref_pInstance = nullptr;
}

void ezMiniAudioSingleton::DetachSoundInstance(ezMiniAudioSoundInstance*& ref_pInstance)
{
  if (ref_pInstance == nullptr)
    return;

  // deactivate looping
  EZ_MA_CHECK(ma_data_source_set_looping(&ref_pInstance->m_Decoder, false));

  ref_pInstance->m_hComponent.Invalidate(); // owner doesn't want to be notified anymore
  // pInstance->pWorld = nullptr; // but keep the world reference for shutdown behavior

  // owner doesn't point to it any longer, but we'll continue playing it
  ref_pInstance = nullptr;
}


void ezMiniAudioSingleton::DetachAndFadeOutSoundInstance(ezMiniAudioSoundInstance*& ref_pInstance, ezTime fadeDuration)
{
  if (ref_pInstance == nullptr)
    return;

  EZ_LOCK(m_pData->m_Mutex);
  m_pData->m_FadingInstances.PushBack(ref_pInstance->m_uiOwnIndex);

  ref_pInstance->m_hComponent.Invalidate(); // owner doesn't want to be notified anymore
  // pInstance->pWorld = nullptr; // but keep the world reference for shutdown behavior

  ma_sound_stop_with_fade_in_milliseconds(&ref_pInstance->m_Sound, static_cast<ma_uint64>(fadeDuration.GetMilliseconds()));

  // owner doesn't point to it any longer, but we'll continue playing it
  ref_pInstance = nullptr;
}

void ezMiniAudioSingleton::SoundEnded(ezMiniAudioSoundInstance* pInstance)
{
  if (pInstance == nullptr)
    return;

  EZ_LOCK(m_pData->m_Mutex);
  EZ_ASSERT_DEBUG(pInstance->m_bInUse, "Sound should still be flagged as in-use");
  m_pData->m_FinishedInstances.PushBack(pInstance->m_uiOwnIndex);
}

void ezMiniAudioSingleton::StopWorldSounds(ezWorld* pWorld)
{
  if (m_pData == nullptr)
    return;

  EZ_LOCK(m_pData->m_Mutex);

  for (ezUInt32 i = 0; i < m_pData->m_FinishedInstances.GetCount();)
  {
    const ezUInt32 idx = m_pData->m_FinishedInstances[i];

    if (m_pData->m_SoundInstancesStorage[idx].pWorld == pWorld)
    {
      m_pData->m_FinishedInstances.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }

  for (ezUInt32 i = 0; i < m_pData->m_FadingInstances.GetCount();)
  {
    const ezUInt32 idx = m_pData->m_FadingInstances[i];

    if (m_pData->m_SoundInstancesStorage[idx].pWorld == pWorld)
    {
      m_pData->m_FadingInstances.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }

  for (ezUInt32 i = 0; i < m_pData->m_SoundInstancesStorage.GetCount(); ++i)
  {
    auto* pInst = &m_pData->m_SoundInstancesStorage[i];

    if (pInst->pWorld == pWorld)
    {
      // since the world pointer is set, that means the sound is not freed
      FreeSoundInstance(pInst);
    }
  }
}
