#include <PCH.h>
#include <FmodPlugin/PluginInterface.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/FmodSingleton.h>
#include <GameEngine/GameApplication/GameApplication.h>

EZ_IMPLEMENT_SINGLETON(ezFmod);

static ezFmod g_FmodSingleton;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) && EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  HANDLE g_hLiveUpdateMutex = NULL;
#endif

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

  EZ_FMOD_ASSERT(FMOD::Studio::System::create(&m_pStudioSystem));

  // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
  EZ_FMOD_ASSERT(m_pStudioSystem->getLowLevelSystem(&m_pLowLevelSystem));
  EZ_FMOD_ASSERT(m_pLowLevelSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0)); /// \todo Hardcoded format

  void *extraDriverData = nullptr;
  FMOD_STUDIO_INITFLAGS studioflags = FMOD_STUDIO_INIT_NORMAL;

  /// \todo fmod live update doesn't work with multiple instances
  // bank loading fails, once two processes are running that use this feature
  // therefore this is deactivated for now
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
    }
#else
    studioflags |= FMOD_STUDIO_INIT_LIVEUPDATE;
#endif
  }
#endif

  const int maxChannels = 32;

  EZ_FMOD_ASSERT(m_pStudioSystem->initialize(maxChannels, studioflags, FMOD_INIT_NORMAL, extraDriverData));
  /// \todo Configure max channels etc.

  if ((studioflags & FMOD_STUDIO_INIT_LIVEUPDATE) != 0)
  {
    ezLog::Success("Fmod Live-Update is enabled for this process.");
  }

  UpdateFmod();
}

void ezFmod::Shutdown()
{
  if (!m_bInitialized)
    return;

  m_bInitialized = false;

  ezResourceManager::FreeUnusedResources(true);

  m_pStudioSystem->release();
  m_pStudioSystem = nullptr;
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

void ezFmod::UpdateFmod()
{
  if (m_pStudioSystem == nullptr)
    return;

  m_pStudioSystem->update();
}

void ezFmod::SetMasterChannelVolume(float volume)
{
  FMOD::ChannelGroup* channel;
  m_pLowLevelSystem->getMasterChannelGroup(&channel);

  channel->setVolume(ezMath::Clamp(volume, 0.0f, 1.0f));
}

float ezFmod::GetMasterChannelVolume() const
{
  FMOD::ChannelGroup* channel;
  m_pLowLevelSystem->getMasterChannelGroup(&channel);

  float volume = 1.0f;
  channel->getVolume(&volume);
  return volume;
}

void ezFmod::SetMasterChannelMute(bool mute)
{
  FMOD::ChannelGroup* channel;
  m_pLowLevelSystem->getMasterChannelGroup(&channel);

  channel->setMute(mute);
}

bool ezFmod::GetMasterChannelMute() const
{
  FMOD::ChannelGroup* channel;
  m_pLowLevelSystem->getMasterChannelGroup(&channel);

  bool mute = false;
  channel->getMute(&mute);

  return mute;
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
  m_VcaVolumes[szVcaGroupGuid] = ezMath::Clamp(volume, 0.0f, 1.0f);

  UpdateSoundGroupVolumes();
}

float ezFmod::GetSoundGroupVolume(const char* szVcaGroupGuid) const
{
  auto it = m_VcaVolumes.Find(szVcaGroupGuid);
  if (it.IsValid())
    return it.Value();

  return 1.0f;
}

void ezFmod::UpdateSoundGroupVolumes()
{
  for (auto it = m_VcaVolumes.GetIterator(); it.IsValid(); ++it)
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

EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_FmodSingleton);

