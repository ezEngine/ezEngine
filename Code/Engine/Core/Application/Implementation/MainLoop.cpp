
#include <Core/PCH.h>
#include <Core/Basics.h>
#include <Core/Application/Application.h>
#include <Foundation/Configuration/Startup.h>

void ezRun(ezApplication* pApplicationInstance)
{
  EZ_ASSERT_ALWAYS(pApplicationInstance != NULL, "ezRun() requires a valid non-null application instance pointer.");
  EZ_ASSERT_ALWAYS(ezApplication::s_pApplicationInstance == NULL, "There can only be one ezApplication.");

  // Set application instance pointer to the supplied instance
  ezApplication::s_pApplicationInstance = pApplicationInstance;

  pApplicationInstance->BeforeEngineInit();

  // this will startup all base and core systems
  // 'EngineStartup' must not be done before a window is available (if at all)
  // so we don't do that here
  ezStartup::StartupCore();

  pApplicationInstance->AfterEngineInit();


  while (pApplicationInstance->Run() == ezApplication::Continue)
  {
  }

  pApplicationInstance->BeforeEngineShutdown();

  ezStartup::ShutdownBase();

  pApplicationInstance->AfterEngineShutdown();

  // Reset application instance so code running after the app will trigger asserts etc. to be cleaned up
  // Destructor is called by entry point function
  ezApplication::s_pApplicationInstance = NULL;
}

EZ_STATICLINK_REFPOINT(Core_Application_Implementation_MainLoop);

