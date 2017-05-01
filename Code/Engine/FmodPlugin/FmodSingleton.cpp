#include <PCH.h>
#include <FmodPlugin/PluginInterface.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>
#include <FmodPlugin/FmodSingleton.h>
#include <GameEngine/GameApplication/GameApplication.h>

EZ_IMPLEMENT_SINGLETON(ezFmod);

static ezFmod g_FmodSingleton;

ezFmod::ezFmod()
  : m_SingletonRegistrar(this)
{
  m_bInitialized = false;

  m_pFmodSystem = nullptr;
  m_pLowLevelSystem = nullptr;
}

void ezFmod::Startup()
{
  if (m_bInitialized)
    return;

  m_bInitialized = true;

  EZ_FMOD_ASSERT(FMOD::Studio::System::create(&m_pFmodSystem));

  // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
  EZ_FMOD_ASSERT(m_pFmodSystem->getLowLevelSystem(&m_pLowLevelSystem));
  EZ_FMOD_ASSERT(m_pLowLevelSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0)); /// \todo Hardcoded format

  void *extraDriverData = nullptr;
  FMOD_STUDIO_INITFLAGS studioflags = FMOD_STUDIO_INIT_NORMAL;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  studioflags |= FMOD_STUDIO_INIT_LIVEUPDATE;
#endif

  const int maxChannels = 32;

  EZ_FMOD_ASSERT(m_pFmodSystem->initialize(maxChannels, studioflags, FMOD_INIT_NORMAL, extraDriverData));
  /// \todo Configure max channels etc.
}

void ezFmod::Shutdown()
{
  if (!m_bInitialized)
    return;

  m_bInitialized = false;

  ezResourceManager::FreeUnusedResources(true);

  m_pFmodSystem->release();
  m_pFmodSystem = nullptr;
}

void ezFmod::SetNumListeners(ezUInt8 uiNumListeners)
{
  EZ_ASSERT_DEV(uiNumListeners <= FMOD_MAX_LISTENERS, "Fmod supports only up to {0} listeners.", FMOD_MAX_LISTENERS);

  m_pFmodSystem->setNumListeners(uiNumListeners);
}

ezUInt8 ezFmod::GetNumListeners()
{
  int i = 0;
  m_pFmodSystem->getNumListeners(&i);
  return i;
}

void ezFmod::UpdateSound()
{
  if (m_pFmodSystem == nullptr)
    return;

  m_pFmodSystem->update();
}


void ezFmod::GameApplicationEventHandler(const ezGameApplicationEvent& e)
{
  if (e.m_Type == ezGameApplicationEvent::Type::BeforeUpdatePlugins)
  {
    ezFmod::GetSingleton()->UpdateSound();
  }
}

void ezFmod::SetNumBlendedReverbVolumes(ezUInt8 uiNumBlendedVolumes)
{
  m_uiNumBlendedVolumes = ezMath::Clamp<ezUInt8>(m_uiNumBlendedVolumes, 0, 4);
}

EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_FmodSingleton);

