#include <SampleGamePluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/UniquePtr.h>

#include <SampleGamePlugin/Interface/SingletonInterface.h>

static ezUniquePtr<PrintImplementation> s_PrintInterface;

// BEGIN-DOCS-CODE-SNIPPET: startup-block
// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(SampleGamePluginStartupGroup, SampleGamePluginMainStartup)

  // list all the subsystems that we want to be initialized first
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation", // all subsystems from the 'Foundation' group (this is redundant, because `Core` already depends on `Foundation`)
    "Core"        // and all subsystems from the 'Core' group
  END_SUBSYSTEM_DEPENDENCIES

  // BEGIN-DOCS-CODE-SNIPPET: singleton-allocate
  ON_CORESYSTEMS_STARTUP
  {
    // allocate an implementation of PrintInterface
    s_PrintInterface = EZ_DEFAULT_NEW(PrintImplementation);

    s_PrintInterface->OnCoreSystemsStartup();
    s_PrintInterface->Print("Called ON_CORESYSTEMS_STARTUP");
  }
  // END-DOCS-CODE-SNIPPET

  // BEGIN-DOCS-CODE-SNIPPET: singleton-deallocate
  ON_CORESYSTEMS_SHUTDOWN
  {
    s_PrintInterface->Print("Called ON_CORESYSTEMS_SHUTDOWN");

    // clean up the s_PrintInterface, otherwise we would get asserts about memory leaks at shutdown
    s_PrintInterface.Clear();
  }
  // END-DOCS-CODE-SNIPPET

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    // we can query 'an implementation of PrintInterface' through the ezSingletonRegistry
    // BEGIN-DOCS-CODE-SNIPPET: singleton-query-interface
    ezSingletonRegistry::GetSingletonInstance<PrintInterface>()->Print("Called ON_HIGHLEVELSYSTEMS_STARTUP");
    // END-DOCS-CODE-SNIPPET
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    // we could also query 'the one instance of the PrintImplementation singleton'
    // BEGIN-DOCS-CODE-SNIPPET: singleton-query-instance
    PrintImplementation::GetSingleton()->Print("Called ON_HIGHLEVELSYSTEMS_SHUTDOWN");
    // END-DOCS-CODE-SNIPPET
  }

EZ_END_SUBSYSTEM_DECLARATION;
// END-DOCS-CODE-SNIPPET
// clang-format on
