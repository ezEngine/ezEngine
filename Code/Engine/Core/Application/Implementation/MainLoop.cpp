
#include <Core/PCH.h>
#include <Core/Basics.h>
#include <Core/Application/Application.h>
#include <Foundation/Configuration/Startup.h>

void ezRun(ezApplication* pApplicationInstance)
{
  EZ_ASSERT_ALWAYS(pApplicationInstance != nullptr, "ezRun() requires a valid non-null application instance pointer.");
  EZ_ASSERT_ALWAYS(ezApplication::s_pApplicationInstance == nullptr, "There can only be one ezApplication.");

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

  ezStartup::ShutdownCore();

  pApplicationInstance->AfterEngineShutdown();

  // Reset application instance so code running after the app will trigger asserts etc. to be cleaned up
  // Destructor is called by entry point function
  ezApplication::s_pApplicationInstance = nullptr;

  if (pApplicationInstance->m_bReportMemoryLeaks)
  {
    ezMemoryTracker::DumpMemoryLeaks();
  }
}

EZ_STATICLINK_FILE(Core, Core_Application_Implementation_MainLoop);

