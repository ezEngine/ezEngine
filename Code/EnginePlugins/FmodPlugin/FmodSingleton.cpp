#include <FmodPlugin/FmodPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Platform/PlatformDesc.h>
#include <GameEngine/GameApplication/GameApplication.h>

EZ_IMPLEMENT_SINGLETON(ezFmod);

static ezFmod g_FmodSingleton;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) && EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
HANDLE g_hLiveUpdateMutex = NULL;
#endif

ezCVarFloat cvar_FmodMasterVolume("FMOD.MasterVolume", 1.0f, ezCVarFlags::Save, "Overall volume for all FMOD output");
ezCVarBool cvar_FmodMute("FMOD.Mute", false, ezCVarFlags::Default, "Whether FMOD output is muted");
ezCVarBool cvar_FmodPause("FMOD.Pause", false, ezCVarFlags::Default, "Whether FMOD output is paused");

ezFmod::ezFmod()
  : m_SingletonRegistrar(this)
{
  m_bInitialized = false;
  m_vListenerPosition.SetZero();

  m_pStudioSystem = nullptr;
  m_pLowLevelSystem = nullptr;
}

void ezFmod::Startup()
{
  if (m_bInitialized)
    return;

  m_pData = EZ_DEFAULT_NEW(Data);

  DetectPlatform();

  if (m_pData->m_Configs.m_AssetProfiles.IsEmpty())
  {
    LoadConfiguration(ezFmodAssetProfiles::s_sConfigFile);

    if (m_pData->m_Configs.m_AssetProfiles.IsEmpty())
    {
      ezLog::Warning("No valid FMOD configuration file available in '{0}'. FMOD will be deactivated.", ezFmodAssetProfiles::s_sConfigFile);
      return;
    }
  }

  if (!m_pData->m_Configs.m_AssetProfiles.Find(m_pData->m_sPlatform).IsValid())
  {
    ezLog::Error("FMOD configuration for platform '{0}' not available. FMOD will be deactivated.", m_pData->m_sPlatform);
    return;
  }

  const auto& config = m_pData->m_Configs.m_AssetProfiles[m_pData->m_sPlatform];

  FMOD_SPEAKERMODE fmodMode = FMOD_SPEAKERMODE_5POINT1;
  {
    ezString sMode = "Unknown";
    switch (config.m_SpeakerMode)
    {
      case ezFmodSpeakerMode::ModeStereo:
        sMode = "Stereo";
        fmodMode = FMOD_SPEAKERMODE_STEREO;
        break;
      case ezFmodSpeakerMode::Mode5Point1:
        sMode = "5.1";
        fmodMode = FMOD_SPEAKERMODE_5POINT1;
        break;
      case ezFmodSpeakerMode::Mode7Point1:
        sMode = "7.1";
        fmodMode = FMOD_SPEAKERMODE_7POINT1;
        break;
    }

    EZ_LOG_BLOCK("FMOD Configuration");
    ezLog::Dev("Platform = '{0}', Mode = {1}, Channels = {2}, SamplerRate = {3}", m_pData->m_sPlatform, sMode, config.m_uiVirtualChannels, config.m_uiSamplerRate);
    ezLog::Dev("Master Bank = '{0}'", config.m_sMasterSoundBank);
  }

  EZ_FMOD_ASSERT(FMOD::Studio::System::create(&m_pStudioSystem));

  // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
  EZ_FMOD_ASSERT(m_pStudioSystem->getCoreSystem(&m_pLowLevelSystem));
  EZ_FMOD_ASSERT(m_pLowLevelSystem->setSoftwareFormat(config.m_uiSamplerRate, fmodMode, 0));

  void* extraDriverData = nullptr;
  FMOD_STUDIO_INITFLAGS studioflags = FMOD_STUDIO_INIT_NORMAL;

  // FMOD live update doesn't work with multiple instances and the same default IP
  // bank loading fails, once two processes are running that use this feature with the same IP
  // this could be reconfigured through the advanced settings, but for now we just enable live update for the first process
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  {
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
    // mutex handle will be closed automatically on process termination
    GetLastError(); // clear any pending error codes
    g_hLiveUpdateMutex = CreateMutexW(nullptr, TRUE, L"ezFmodLiveUpdate");

    DWORD err = GetLastError();
    if (g_hLiveUpdateMutex != NULL && err != ERROR_ALREADY_EXISTS)
    {
      studioflags |= FMOD_STUDIO_INIT_LIVEUPDATE;
    }
    else
    {
      ezLog::Warning("FMOD Live-Update not available for this process, another process using FMOD is already running.");
      if (g_hLiveUpdateMutex != NULL)
      {
        CloseHandle(g_hLiveUpdateMutex); // we didn't create it, so don't keep it alive
        g_hLiveUpdateMutex = NULL;
      }
    }
#  endif
  }
#endif

  EZ_FMOD_ASSERT(m_pStudioSystem->initialize(config.m_uiVirtualChannels, studioflags, FMOD_INIT_NORMAL, extraDriverData));

  if ((studioflags & FMOD_STUDIO_INIT_LIVEUPDATE) != 0)
  {
    ezLog::Success("FMOD Live-Update is enabled for this process.");
  }

  if (LoadMasterSoundBank(config.m_sMasterSoundBank).Failed())
  {
    ezLog::Error("Failed to load FMOD master sound bank '{0}'. Sounds will not play.", config.m_sMasterSoundBank);
    return;
  }

  m_bInitialized = true;

  UpdateSound();
}

void ezFmod::Shutdown()
{
  if (m_bInitialized)
  {
    // delete all FMOD resources, except the master bank
    ezResourceManager::FreeAllUnusedResources();

    m_bInitialized = false;
    m_pData->m_hMasterBank.Invalidate();
    m_pData->m_hMasterBankStrings.Invalidate();

    // now also delete the master bank
    ezResourceManager::FreeAllUnusedResources();

    // now actually delete the sound bank data
    ClearSoundBankDataDeletionQueue();

    if (m_pStudioSystem != nullptr)
    {
      m_pStudioSystem->release();
      m_pStudioSystem = nullptr;
      m_pLowLevelSystem = nullptr;
    }
  }

  // finally delete all data
  m_pData.Clear();
}

void ezFmod::SetNumListeners(ezUInt8 uiNumListeners)
{
  EZ_ASSERT_DEV(uiNumListeners <= FMOD_MAX_LISTENERS, "FMOD supports only up to {0} listeners.", FMOD_MAX_LISTENERS);

  m_pStudioSystem->setNumListeners(uiNumListeners);
}

ezUInt8 ezFmod::GetNumListeners()
{
  int i = 0;
  m_pStudioSystem->getNumListeners(&i);
  return static_cast<ezUInt8>(i);
}

void ezFmod::LoadConfiguration(ezStringView sFile)
{
  m_pData->m_Configs.Load(sFile).IgnoreResult();
}

void ezFmod::SetOverridePlatform(ezStringView sPlatform)
{
  m_pData->m_sPlatform = sPlatform;
}

void ezFmod::UpdateSound()
{
  if (m_pStudioSystem == nullptr)
    return;

  EZ_ASSERT_DEV(m_pData != nullptr, "UpdateSound() should not be called at this time.");

  // make sure to reload the sound bank, if it has been unloaded
  if (m_pData->m_hMasterBank.IsValid())
  {
    ezResourceLock<ezFmodSoundBankResource> pMaster(m_pData->m_hMasterBank, ezResourceAcquireMode::BlockTillLoaded);
  }

  // Master Volume
  {
    FMOD::ChannelGroup* channel;
    m_pLowLevelSystem->getMasterChannelGroup(&channel);
    channel->setVolume(ezMath::Clamp<float>(cvar_FmodMasterVolume, 0.0f, 1.0f));
  }

  // Mute
  {
    FMOD::ChannelGroup* channel;
    m_pLowLevelSystem->getMasterChannelGroup(&channel);

    channel->setMute(cvar_FmodMute);
  }

  // Pause
  {
    FMOD::ChannelGroup* channel;
    m_pLowLevelSystem->getMasterChannelGroup(&channel);

    channel->setPaused(cvar_FmodPause);
  }

  m_pStudioSystem->update();

  ClearSoundBankDataDeletionQueue();
}

void ezFmod::SetMasterChannelVolume(float fVolume)
{
  cvar_FmodMasterVolume = ezMath::Clamp<float>(fVolume, 0.0f, 1.0f);
}

float ezFmod::GetMasterChannelVolume() const
{
  return cvar_FmodMasterVolume;
}

void ezFmod::SetMasterChannelMute(bool bMute)
{
  cvar_FmodMute = bMute;
}

bool ezFmod::GetMasterChannelMute() const
{
  return cvar_FmodMute;
}

void ezFmod::SetMasterChannelPaused(bool bPaused)
{
  cvar_FmodPause = bPaused;
}

bool ezFmod::GetMasterChannelPaused() const
{
  return cvar_FmodPause;
}

void ezFmod::SetSoundGroupVolume(ezStringView sVcaGroupGuid, float fVolume)
{
  m_pData->m_VcaVolumes[sVcaGroupGuid] = ezMath::Clamp(fVolume, 0.0f, 1.0f);

  UpdateSoundGroupVolumes();
}

float ezFmod::GetSoundGroupVolume(ezStringView sVcaGroupGuid) const
{
  return m_pData->m_VcaVolumes.GetValueOrDefault(sVcaGroupGuid, 1.0f);
}

void ezFmod::UpdateSoundGroupVolumes()
{
  for (auto it = m_pData->m_VcaVolumes.GetIterator(); it.IsValid(); ++it)
  {
    FMOD::Studio::VCA* pVca = nullptr;
    m_pStudioSystem->getVCA(it.Key().GetData(), &pVca);

    if (pVca != nullptr)
    {
      pVca->setVolume(it.Value());
    }
  }
}

void ezFmod::GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e)
{
  if (e.m_Type == ezGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    ezFmod::GetSingleton()->UpdateSound();
  }
}

void ezFmod::SetNumBlendedReverbVolumes(ezUInt8 uiNumBlendedVolumes)
{
  m_uiNumBlendedVolumes = ezMath::Clamp<ezUInt8>(m_uiNumBlendedVolumes, 0, 4);
}

void ezFmod::SetGlobalParameter(const char* szName, float fValue)
{
  m_pStudioSystem->setParameterByName(szName, fValue);
}

float ezFmod::GetGlobalParameter(const char* szName)
{
  float fValue = 0.0f;
  float fFinalValue = 0.0f;
  m_pStudioSystem->getParameterByName(szName, &fValue, &fFinalValue);
  return fFinalValue;
}

void ezFmod::SetListenerOverrideMode(bool bEnabled)
{
  m_bListenerOverrideMode = bEnabled;
}

void ezFmod::SetListener(ezInt32 iIndex, const ezVec3& vPosition, const ezVec3& vForward, const ezVec3& vUp, const ezVec3& vVelocity)
{
  if (m_bListenerOverrideMode)
  {
    if (iIndex != -1)
      return;

    iIndex = 0;
  }

  if (iIndex < 0 || iIndex >= FMOD_MAX_LISTENERS)
    return;

  if (iIndex == 0)
  {
    m_vListenerPosition = vPosition;
  }

  FMOD_3D_ATTRIBUTES attr;
  attr.position.x = vPosition.x;
  attr.position.y = vPosition.y;
  attr.position.z = vPosition.z;
  attr.forward.x = vForward.x;
  attr.forward.y = vForward.y;
  attr.forward.z = vForward.z;
  attr.up.x = vUp.x;
  attr.up.y = vUp.y;
  attr.up.z = vUp.z;
  attr.velocity.x = vVelocity.x;
  attr.velocity.y = vVelocity.y;
  attr.velocity.z = vVelocity.z;

  if (m_pStudioSystem != nullptr)
  {
    m_pStudioSystem->setListenerAttributes(iIndex, &attr);
  }
}

ezResult ezFmod::OneShotSound(ezWorld* pWorld, ezStringView sResourceID, const ezTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  ezFmodSoundEventResourceHandle hSound = ezResourceManager::LoadResource<ezFmodSoundEventResource>(sResourceID);

  if (!hSound.IsValid())
    return EZ_FAILURE;

  ezResourceLock<ezFmodSoundEventResource> pSound(hSound, bBlockIfNotLoaded ? ezResourceAcquireMode::BlockTillLoaded_NeverFail : ezResourceAcquireMode::AllowLoadingFallback_NeverFail);

  if (pSound.GetAcquireResult() != ezResourceAcquireResult::Final)
    return EZ_FAILURE;

  return pSound->PlayOnce(globalPosition, fPitch, fVolume);
}

void ezFmod::DetectPlatform()
{
  if (!m_pData->m_sPlatform.IsEmpty())
    return;

  m_pData->m_sPlatform = ezPlatformDesc::GetThisPlatformDesc().GetType();
}

ezResult ezFmod::LoadMasterSoundBank(const char* szMasterBankResourceID)
{
  if (ezStringUtils::IsNullOrEmpty(szMasterBankResourceID))
  {
    ezLog::Error("FMOD master bank name has not been configured.");
    return EZ_FAILURE;
  }

  m_pData->m_hMasterBank = ezResourceManager::LoadResource<ezFmodSoundBankResource>(szMasterBankResourceID);

  {
    ezResourceLock<ezFmodSoundBankResource> pResource(m_pData->m_hMasterBank, ezResourceAcquireMode::BlockTillLoaded);

    if (pResource.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
      return EZ_FAILURE;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezStringBuilder sStringsBankPath = szMasterBankResourceID;
  sStringsBankPath.ChangeFileExtension("strings.bank");

  m_pData->m_hMasterBankStrings = ezResourceManager::LoadResource<ezFmodSoundBankResource>(sStringsBankPath);

  {
    ezResourceLock<ezFmodSoundBankResource> pResource(m_pData->m_hMasterBankStrings, ezResourceAcquireMode::BlockTillLoaded);

    if (pResource.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
      return EZ_FAILURE;
  }
#endif

  return EZ_SUCCESS;
}

void ezFmod::QueueSoundBankDataForDeletion(ezDataBuffer* pData)
{
  EZ_LOCK(m_DeletionQueueMutex);

  m_pData->m_SbDeletionQueue.PushBack(pData);
}

void ezFmod::ClearSoundBankDataDeletionQueue()
{
  if (m_pData->m_SbDeletionQueue.IsEmpty())
    return;

  EZ_LOCK(m_DeletionQueueMutex);

  if (m_pStudioSystem != nullptr)
  {
    // make sure the data is not in use anymore
    m_pStudioSystem->flushCommands();
  }

  for (auto pData : m_pData->m_SbDeletionQueue)
  {
    EZ_DEFAULT_DELETE(pData);
  }

  m_pData->m_SbDeletionQueue.Clear();
}

EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_FmodSingleton);
