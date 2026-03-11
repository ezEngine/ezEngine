#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>

#if defined(LIVEPP_ENABLED)
#  include <LPP_API_x64_CPP.h>
inline bool allow_hotreload = false;
inline lpp::LppDefaultAgent lppAgent;
#endif

ezResult ezRun_Startup(ezApplication* pApplicationInstance)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) && defined(LIVEPP_ENABLED)
  // create a synchronized agent, loading the Live++ agent from the given path, e.g. "ThirdParty/LivePP"
  lppAgent = lpp::LppCreateDefaultAgent(nullptr, L"LivePP");
  // bail out in case the agent is not valid
  if (!lpp::LppIsValidDefaultAgent(&lppAgent))
  {
    ezLog::Warning("Failed to create Live++ agent.");
  }
  else
  {
    ezLog::Info("Live++ agent created.");
    allow_hotreload = true;
    lppAgent.EnableModule(lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_NONE, nullptr, nullptr);
    // make Live++ handle dynamically loaded modules automatically, enabling them on load, disabling them on unload
    lppAgent.EnableAutomaticHandlingOfDynamicallyLoadedModules(nullptr, nullptr);
  }
#endif
  EZ_ASSERT_ALWAYS(pApplicationInstance != nullptr, "ezRun() requires a valid non-null application instance pointer.");
  EZ_ASSERT_ALWAYS(ezApplication::s_pApplicationInstance == nullptr, "There can only be one ezApplication.");

  // Set application instance pointer to the supplied instance
  ezApplication::s_pApplicationInstance = pApplicationInstance;

  EZ_SUCCEED_OR_RETURN(pApplicationInstance->BeforeCoreSystemsStartup());

  // this will startup all base and core systems
  // 'StartupHighLevelSystems' must not be done before a window is available (if at all)
  // so we don't do that here
  ezStartup::StartupCoreSystems();

  pApplicationInstance->AfterCoreSystemsStartup();
  return EZ_SUCCESS;
}

void ezRun_MainLoop(ezApplication* pApplicationInstance)
{
  while (!pApplicationInstance->ShouldApplicationQuit())
  {
    pApplicationInstance->Run();
  }
}

void ezRun_Shutdown(ezApplication* pApplicationInstance)
{
  // high level systems shutdown
  // may do nothing, if the high level systems were never initialized
  {
    pApplicationInstance->BeforeHighLevelSystemsShutdown();
    ezStartup::ShutdownHighLevelSystems();
    pApplicationInstance->AfterHighLevelSystemsShutdown();
  }

  // core systems shutdown
  {
    pApplicationInstance->BeforeCoreSystemsShutdown();
    ezStartup::ShutdownCoreSystems();
    pApplicationInstance->AfterCoreSystemsShutdown();
  }

  // Flush standard output to make log available.
  fflush(stdout);
  fflush(stderr);

  // Reset application instance so code running after the app will trigger asserts etc. to be cleaned up
  // Destructor is called by entry point function
  ezApplication::s_pApplicationInstance = nullptr;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) && defined(LIVEPP_ENABLED)
  // destroy the Live++ agent
  lpp::LppDestroyDefaultAgent(&lppAgent);
#endif

  // memory leak reporting cannot be done here, because the application instance is still alive and may still hold on to memory that needs
  // to be freed first
}

void ezRun(ezApplication* pApplicationInstance)
{
  if (ezRun_Startup(pApplicationInstance).Succeeded())
  {
    ezRun_MainLoop(pApplicationInstance);
  }
  ezRun_Shutdown(pApplicationInstance);
}
