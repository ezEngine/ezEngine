
#include <Core/PCH.h>
#include <Core/Application/Application.h>

void ezRun(ezApplication* pApplicationInstance)
{
  EZ_ASSERT_ALWAYS(pApplicationInstance != nullptr, "ezRun() requires a valid non-null application instance pointer.");
  EZ_ASSERT_ALWAYS(ezApplication::s_pApplicationInstance == nullptr, "There can only be one ezApplication.");

  // Set application instance pointer to the supplied instance
  ezApplication::s_pApplicationInstance = pApplicationInstance;

  pApplicationInstance->BeforeCoreStartup();

  // this will startup all base and core systems
  // 'EngineStartup' must not be done before a window is available (if at all)
  // so we don't do that here
  ezStartup::StartupCore();

  pApplicationInstance->AfterCoreStartup();

  while (pApplicationInstance->Run() == ezApplication::Continue)
  {
  }

  pApplicationInstance->BeforeCoreShutdown();

  ezStartup::ShutdownCore();

  pApplicationInstance->AfterCoreShutdown();

  // Flush standard output to make log available.
  fflush(stdout);
  fflush(stderr);

  // Reset application instance so code running after the app will trigger asserts etc. to be cleaned up
  // Destructor is called by entry point function
  ezApplication::s_pApplicationInstance = nullptr;

  // memory leak reporting cannot be done here, because the application instance is still alive and may still hold on to memory that needs to be freed first
}

EZ_STATICLINK_FILE(Core, Core_Application_Implementation_MainLoop);

