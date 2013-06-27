
#include <Core/PCH.h>
#include <Core/Basics.h>
#include <Core/Application/Application.h>


void ezRun(ezApplication* pApplicationInstance)
{
  EZ_ASSERT_ALWAYS(pApplicationInstance != NULL, "ezRun() requires a valid non-null application instance pointer");

  // Set application instance pointer to the supplied instance
  ezApplication::s_pApplicationInstance = pApplicationInstance;

  pApplicationInstance->BeforeEngineInit();

  ezFoundation::Initialize();

  pApplicationInstance->AfterEngineInit();


  while(pApplicationInstance->Run())
  {
  }

  pApplicationInstance->BeforeEngineShutdown();

  ezFoundation::Shutdown();

  pApplicationInstance->AfterEngineShutdown();

  // Reset application instance so code running after the app will trigger asserts etc. to be cleaned up
  // Destructor is called by entry point function
  ezApplication::s_pApplicationInstance = NULL;
}