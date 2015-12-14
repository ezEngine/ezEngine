#include <FmodPlugin/PCH.h>
#include <FmodPlugin/PluginInterface.h>
#include <FmodPlugin/FmodSceneModule.h>
#include <Foundation/Configuration/AbstractInterfaceRegistry.h>

static ezFmod g_FmodSingleton;

ezFmod::ezFmod()
{
  m_bInitialized = false;

  m_pFmodSystem = nullptr;
  m_pLowLevelSystem = nullptr;
}

ezFmod* ezFmod::GetSingleton()
{
  return &g_FmodSingleton;
}

std::string Common_MediaPath(const char* sz)
{
  //std::string s = "C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/studio/examples/media/";
  std::string s = "C:/Users/jakra/Documents/FMOD Studio/examples/Build/Desktop/";
  s += sz;

  return s;
}

void ezFmod::Startup()
{
  if (m_bInitialized)
    return;

  m_bInitialized = true;

  ezAbstractInterfaceRegistry::RegisterInterfaceImplementation("ezFmodInterface", this);

  EZ_FMOD_ASSERT(FMOD::Studio::System::create(&m_pFmodSystem));

  // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
  EZ_FMOD_ASSERT(m_pFmodSystem->getLowLevelSystem(&m_pLowLevelSystem));
  EZ_FMOD_ASSERT(m_pLowLevelSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));

  void *extraDriverData = nullptr;
  EZ_FMOD_ASSERT(m_pFmodSystem->initialize(32, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extraDriverData));



  FMOD::Studio::Bank* masterBank = NULL;
  EZ_FMOD_ASSERT(m_pFmodSystem->loadBankFile(Common_MediaPath("Master Bank.bank").c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &masterBank));

  FMOD::Studio::Bank* stringsBank = NULL;
  EZ_FMOD_ASSERT(m_pFmodSystem->loadBankFile(Common_MediaPath("Master Bank.strings.bank").c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank));

  FMOD::Studio::Bank* ambienceBank = NULL;
  EZ_FMOD_ASSERT(m_pFmodSystem->loadBankFile(Common_MediaPath("Surround_Ambience.bank").c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &ambienceBank));

  FMOD::Studio::Bank* menuBank = NULL;
  EZ_FMOD_ASSERT(m_pFmodSystem->loadBankFile(Common_MediaPath("UI_Menu.bank").c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &menuBank));

  FMOD::Studio::Bank* weaponsBank = NULL;
  EZ_FMOD_ASSERT(m_pFmodSystem->loadBankFile(Common_MediaPath("Weapons.bank").c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &weaponsBank));

}

void ezFmod::Shutdown()
{
  if (!m_bInitialized)
    return;

  m_bInitialized = false;


  m_pFmodSystem->release();
  m_pFmodSystem = nullptr;

  ezAbstractInterfaceRegistry::UnregisterInterfaceImplementation("ezFmodInterface", this);
}

void ezFmod::SetNumListeners(ezUInt8 uiNumListeners)
{
  EZ_ASSERT_DEV(uiNumListeners <= FMOD_MAX_LISTENERS, "Fmod supports only up to %u listeners.", FMOD_MAX_LISTENERS);

  m_pFmodSystem->setNumListeners(uiNumListeners);
}

ezUInt8 ezFmod::GetNumListeners()
{
  int i = 0;
  m_pFmodSystem->getNumListeners(&i);
  return i;
}
