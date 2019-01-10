#include <PCH.h>

#include <Core/Application/Application.h>
#include <Foundation/Configuration/Startup.h>

void ezRun_Startup(ezApplication* pApplicationInstance)
{
  EZ_ASSERT_ALWAYS(pApplicationInstance != nullptr, "ezRun() requires a valid non-null application instance pointer.");
  EZ_ASSERT_ALWAYS(ezApplication::s_pApplicationInstance == nullptr, "There can only be one ezApplication.");

  // Set application instance pointer to the supplied instance
  ezApplication::s_pApplicationInstance = pApplicationInstance;

  pApplicationInstance->BeforeCoreSystemsStartup();

  // this will startup all base and core systems
  // 'StartupHighLevelSystems' must not be done before a window is available (if at all)
  // so we don't do that here
  ezStartup::StartupCoreSystems();

  pApplicationInstance->AfterCoreSystemsStartup();
}

void ezRun_MainLoop(ezApplication* pApplicationInstance)
{
  while (pApplicationInstance->Run() == ezApplication::Continue)
  {
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

  // memory leak reporting cannot be done here, because the application instance is still alive and may still hold on to memory that needs
  // to be freed first
}

void ezRun(ezApplication* pApplicationInstance)
{
  ezRun_Startup(pApplicationInstance);
  ezRun_MainLoop(pApplicationInstance);
  ezRun_Shutdown(pApplicationInstance);
}

EZ_STATICLINK_FILE(Core, Core_Application_Implementation_MainLoop);
