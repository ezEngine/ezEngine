#include <PCH.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/FmodSingleton.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <FmodPlugin/FmodIncludes.h>
#include <Foundation/Configuration/CVar.h>

EZ_IMPLEMENT_SINGLETON(ezFmod);

static ezFmod g_FmodSingleton;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) && EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
HANDLE g_hLiveUpdateMutex = NULL;
#endif

ezCVarFloat g_FmodMasterVolume("fmod_MasterVolume", 1.0f, ezCVarFlags::Default, "Master volume for all fmod output");
ezCVarBool g_FmodMute("fmod_Mute", false, ezCVarFlags::Default, "Whether Fmod sound output is muted");

ezFmod::ezFmod()
  : m_SingletonRegistrar(this)
{
  m_bInitialized = false;

  m_pStudioSystem = nullptr;
  m_pLowLevelSystem = nullptr;
}

void ezFmod::Startup()
{
  if (m_bInitialized)
    return;

  m_bInitialized = true;

  m_pData = EZ_DEFAULT_NEW(Data);

  DetectPlatform();

  if (m_pData->m_Configs.m_PlatformConfigs.IsEmpty())
  {
    const char* szFile = ":project/FmodConfig.ddl";
    LoadConfiguration(szFile);

    if (m_pData->m_Configs.m_PlatformConfigs.IsEmpty())
    {
      ezLog::Warning("No valid fmod configuration file available in '{0}'. Fmod will be deactivated.", szFile);
      return;
    }
  }

  if (!m_pData->m_Configs.m_PlatformConfigs.Find(m_pData->m_sPlatform).IsValid())
  {
    ezLog::Error("Fmod configuration for platform '{0}' not available. Fmod will be deactivated.", m_pData->m_sPlatform);
    return;
  }

  const auto& config = m_pData->m_Configs.m_PlatformConfigs[m_pData->m_sPlatform];

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

    EZ_LOG_BLOCK("Fmod Configuration");
    ezLog::Dev("Platform = '{0}', Mode = {1}, Channels = {2}, SamplerRate = {3}", m_pData->m_sPlatform, sMode, config.m_uiVirtualChannels, config.m_uiSamplerRate);
    ezLog::Dev("Master Bank = '{0}'", config.m_sMasterSoundBank);
  }

  EZ_FMOD_ASSERT(FMOD::Studio::System::create(&m_pStudioSystem));

  // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
  EZ_FMOD_ASSERT(m_pStudioSystem->getLowLevelSystem(&m_pLowLevelSystem));
  EZ_FMOD_ASSERT(m_pLowLevelSystem->setSoftwareFormat(config.m_uiSamplerRate, fmodMode, 0));

  void *extraDriverData = nullptr;
  FMOD_STUDIO_INITFLAGS studioflags = FMOD_STUDIO_INIT_NORMAL;

  // fmod live update doesn't work with multiple instances and the same default IP
  // bank loading fails, once two processes are running that use this feature with the same IP
  // this could be reconfigured through the advanced settings, but for now we just enable live update for the first process
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    // mutex handle will be closed automatically on process termination
    GetLastError();
    g_hLiveUpdateMutex = CreateMutexW(nullptr, TRUE, L"ezFmodLiveUpdate");

    if (g_hLiveUpdateMutex != NULL && GetLastError() != ERROR_ALREADY_EXISTS)
    {
      studioflags |= FMOD_STUDIO_INIT_LIVEUPDATE;
    }
    else
    {
      ezLog::Warning("Fmod Live-Update not available for this process, another process using fmod is already running.");
      CloseHandle(g_hLiveUpdateMutex); // we didn't create it, so don't keep it alive
      g_hLiveUpdateMutex = NULL;
    }
#else
    studioflags |= FMOD_STUDIO_INIT_LIVEUPDATE;
#endif
    }
#endif

  EZ_FMOD_ASSERT(m_pStudioSystem->initialize(config.m_uiVirtualChannels, studioflags, FMOD_INIT_NORMAL, extraDriverData));

  if ((studioflags & FMOD_STUDIO_INIT_LIVEUPDATE) != 0)
  {
    ezLog::Success("Fmod Live-Update is enabled for this process.");
  }

  if (LoadMasterSoundBank(config.m_sMasterSoundBank).Failed())
  {
    ezLog::Error("Failed to load fmod master sound bank '{0}'. Sounds will not play.", config.m_sMasterSoundBank);
    return;
  }

  UpdateFmod();
  }

void ezFmod::Shutdown()
{
  if (!m_bInitialized)
    return;

  m_bInitialized = false;
  m_pData.Reset();

  ezResourceManager::FreeUnusedResources(true);

  m_pStudioSystem->release();
  m_pStudioSystem = nullptr;
  m_pLowLevelSystem = nullptr;
}

void ezFmod::SetNumListeners(ezUInt8 uiNumListeners)
{
  EZ_ASSERT_DEV(uiNumListeners <= FMOD_MAX_LISTENERS, "Fmod supports only up to {0} listeners.", FMOD_MAX_LISTENERS);

  m_pStudioSystem->setNumListeners(uiNumListeners);
}

ezUInt8 ezFmod::GetNumListeners()
{
  int i = 0;
  m_pStudioSystem->getNumListeners(&i);
  return i;
}

void ezFmod::LoadConfiguration(const char* szFile)
{
  m_pData->m_Configs.Load(szFile);
}

void ezFmod::SetOverridePlatform(const char* szPlatform)
{
  m_pData->m_sPlatform = szPlatform;
}

void ezFmod::UpdateFmod()
{
  if (m_pStudioSystem == nullptr)
    return;

  // make sure to reload the sound bank, if it has been unloaded
  if (m_pData->m_hMasterBank.IsValid())
  {
    ezResourceLock<ezFmodSoundBankResource> pMaster(m_pData->m_hMasterBank, ezResourceAcquireMode::NoFallback);
  }

  // Master Volume
  {
    FMOD::ChannelGroup* channel;
    m_pLowLevelSystem->getMasterChannelGroup(&channel);
    channel->setVolume(ezMath::Clamp<float>(g_FmodMasterVolume, 0.0f, 1.0f));
  }

  // Mute
  {
    FMOD::ChannelGroup* channel;
    m_pLowLevelSystem->getMasterChannelGroup(&channel);

    channel->setMute(g_FmodMute);
  }

  m_pStudioSystem->update();
}

void ezFmod::SetMasterChannelVolume(float volume)
{
  g_FmodMasterVolume = ezMath::Clamp<float>(volume, 0.0f, 1.0f);
}

float ezFmod::GetMasterChannelVolume() const
{
  return g_FmodMasterVolume;
}

void ezFmod::SetMasterChannelMute(bool mute)
{
  g_FmodMute = mute;
}

bool ezFmod::GetMasterChannelMute() const
{
  return g_FmodMute;
}

void ezFmod::SetMasterChannelPaused(bool paused)
{
  FMOD::ChannelGroup* channel;
  m_pLowLevelSystem->getMasterChannelGroup(&channel);

  channel->setPaused(paused);
}

bool ezFmod::GetMasterChannelPaused() const
{
  FMOD::ChannelGroup* channel;
  m_pLowLevelSystem->getMasterChannelGroup(&channel);

  bool paused = false;
  channel->getPaused(&paused);

  return paused;
}

void ezFmod::SetSoundGroupVolume(const char* szVcaGroupGuid, float volume)
{
  m_pData->m_VcaVolumes[szVcaGroupGuid] = ezMath::Clamp(volume, 0.0f, 1.0f);

  UpdateSoundGroupVolumes();
}

float ezFmod::GetSoundGroupVolume(const char* szVcaGroupGuid) const
{
  auto it = m_pData->m_VcaVolumes.Find(szVcaGroupGuid);
  if (it.IsValid())
    return it.Value();

  return 1.0f;
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

void ezFmod::GameApplicationEventHandler(const ezGameApplicationEvent& e)
{
  if (e.m_Type == ezGameApplicationEvent::Type::BeforeUpdatePlugins)
  {
    ezFmod::GetSingleton()->UpdateFmod();
  }
}

void ezFmod::SetNumBlendedReverbVolumes(ezUInt8 uiNumBlendedVolumes)
{
  m_uiNumBlendedVolumes = ezMath::Clamp<ezUInt8>(m_uiNumBlendedVolumes, 0, 4);
}

void ezFmod::SetListenerOverrideMode(bool enabled)
{
  m_bListenerOverrideMode = enabled;
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

void ezFmod::DetectPlatform()
{
  if (!m_pData->m_sPlatform.IsEmpty())
    return;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  m_pData->m_sPlatform = "Desktop";

#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  m_pData->m_sPlatform = "Desktop"; /// \todo Need to detect mobile device mode

#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
  m_pData->m_sPlatform = "Desktop"; /// \todo Need to detect mobile device mode (Android)

#elif EZ_ENABLED(EZ_PLATFORM_OSX)
  m_pData->m_sPlatform = "Desktop";

#elif EZ_ENABLED(EZ_PLATFORM_IOS)
  m_pData->m_sPlatform = "Mobile";

#elif
#error "Unknown Platform"

#endif
}

ezResult ezFmod::LoadMasterSoundBank(const char* szMasterBankResourceID)
{
  if (ezStringUtils::IsNullOrEmpty(szMasterBankResourceID))
  {
    ezLog::Error("Fmod master bank name has not been configured.");
    return EZ_FAILURE;
  }

  m_pData->m_hMasterBank = ezResourceManager::LoadResource<ezFmodSoundBankResource>(szMasterBankResourceID);

  ezResourceLock<ezFmodSoundBankResource> pResource(m_pData->m_hMasterBank, ezResourceAcquireMode::NoFallback);

  if (pResource->IsMissingResource())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_FmodSingleton);
