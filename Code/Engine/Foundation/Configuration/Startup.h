#pragma once

#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/Configuration/Implementation/StartupBasics.h>
#include <Foundation/Containers/Deque.h>

#define EZ_GLOBALEVENT_STARTUP_CORE_BEGIN "ezStartup_StartupCore_Begin"
#define EZ_GLOBALEVENT_STARTUP_CORE_END "ezStartup_StartupCore_End"
#define EZ_GLOBALEVENT_SHUTDOWN_CORE_BEGIN "ezStartup_ShutdownCore_Begin"
#define EZ_GLOBALEVENT_SHUTDOWN_CORE_END "ezStartup_ShutdownCore_End"

#define EZ_GLOBALEVENT_STARTUP_ENGINE_BEGIN "ezStartup_StartupEngine_Begin"
#define EZ_GLOBALEVENT_STARTUP_ENGINE_END "ezStartup_StartupEngine_End"
#define EZ_GLOBALEVENT_SHUTDOWN_ENGINE_BEGIN "ezStartup_ShutdownEngine_Begin"
#define EZ_GLOBALEVENT_SHUTDOWN_ENGINE_END "ezStartup_ShutdownEngine_End"

#define EZ_GLOBALEVENT_UNLOAD_MODULE_BEGIN "ezStartup_UnloadModule_Begin"
#define EZ_GLOBALEVENT_UNLOAD_MODULE_END "ezStartup_UnloadModule_End"

/// \brief The startup system makes sure to initialize and shut down all known subsystems in the proper order.
/// Each subsystem can define on which other subsystems it is dependent (ie. which other code it needs in an initialized
/// state, before it can run itself). The startup system will sort all subsystems by their dependencies and then initialize
/// them in the proper order.
/// The startup and shutdown sequence consists of two steps. First the 'core' functionality is initialized. This is usually
/// all the functionality that does not depend on a working rendering context.
/// After all systems have had their 'core' functionality initialized, the 'engine' functionality can be initialized.
/// In between these steps the rendering context should be created.
/// Tools that might not create a window or do not want to actually load GPU resources, might get away with only doing
/// the 'core' initialization.
///
/// A subsystem startup configuration needs to be put in some cpp file of the subsystem and looks like this:
///
///EZ_BEGIN_SUBSYSTEM_DECLARATION(ExampleModule, ExampleSubSystem)
///
///  BEGIN_SUBSYSTEM_DEPENDENCIES
///    "SomeOtherSubSystem",
///    "SomeOtherSubSystem2"
///  END_SUBSYSTEM_DEPENDENCIES
///
///  ON_CORE_STARTUP
///  {
///    ezExampleSubSystem::BasicStartup();
///  }
///
///  ON_CORE_SHUTDOWN
///  {
///    ezExampleSubSystem::BasicShutdown();
///  }
///
///  ON_ENGINE_STARTUP
///  {
///    ezExampleSubSystem::EngineStartup();
///  }
///
///  ON_ENGINE_SHUTDOWN
///  {
///    ezExampleSubSystem::EngineShutdown();
///  }
///
///EZ_END_SUBSYSTEM_DECLARATION
///
/// This will automatically register the subsystem, once the code is being loaded (can be dynamically loaded from a DLL).
/// The next time any of the ezStartup functions are called (StartupCore, StartupEngine) the subsystem will be initialized.
///
/// If you need to dynamically unload a DLL and all subsystems within it, you can use ezStartup::UnloadModuleSubSystems
/// to shutdown all subsystems from that module and other subsystems that depend on it.
///
/// All startup / shutdown procedures broadcast global events before and after they execute.
class EZ_FOUNDATION_DLL ezStartup
{
public:

  // 'Base Startup' happens even before 'Core Startup', but only really low level stuff should  be done there
  // and those subsystems should not have any dependencies on each other.
  // 'Base Startup' is automatically done right before 'Core Startup'

  /// \brief Runs the 'base' startup sequence of all subsystems in the proper order.
  static void StartupBase() { Startup(ezStartupStage::Base); }

  /// \brief Runs the 'base' shutdown sequence of all subsystems in the proper order (reversed startup order).
  /// Makes sure that the 'core' shutdown has been run first.
  static void ShutdownBase() { Shutdown(ezStartupStage::Base); }

  /// \brief Runs the 'core' startup sequence of all subsystems in the proper order.
  /// Broadcasts the global event EZ_GLOBALEVENT_STARTUP_CORE_BEGIN and EZ_GLOBALEVENT_STARTUP_CORE_END
  static void StartupCore() { Startup(ezStartupStage::Core); }

  /// \brief Runs the 'core' shutdown sequence of all subsystems in the proper order (reversed startup order).
  /// Makes sure that the 'engine' shutdown has been run first.
  /// Broadcasts the global event EZ_GLOBALEVENT_SHUTDOWN_CORE_BEGIN and EZ_GLOBALEVENT_SHUTDOWN_CORE_END
  static void ShutdownCore() { Shutdown(ezStartupStage::Core); }

  /// \brief Runs the 'engine' startup sequence of all subsystems in the proper order.
  /// Makes sure that the 'core' initialization has been run first.
  /// Broadcasts the global event EZ_GLOBALEVENT_STARTUP_ENGINE_BEGIN and EZ_GLOBALEVENT_STARTUP_ENGINE_END
  static void StartupEngine() { Startup(ezStartupStage::Engine); }

  /// \brief Runs the 'core' shutdown sequence of all subsystems in the proper order (reversed startup order).
  /// Broadcasts the global event EZ_GLOBALEVENT_SHUTDOWN_ENGINE_BEGIN and EZ_GLOBALEVENT_SHUTDOWN_ENGINE_END
  static void ShutdownEngine() { Shutdown(ezStartupStage::Engine); }

  /// \brief Unloads all subsystems from the given module AND all subsystems that directly or indirectly depend on them.
  /// This can be used to shutdown all systems from certain DLLs before that DLL is unloaded (and possibly reloaded).
  /// Broadcasts the global event EZ_GLOBALEVENT_UNLOAD_MODULE_BEGIN and EZ_GLOBALEVENT_UNLOAD_MODULE_END and passes szModuleName in the first event parameter.
  static void UnloadModuleSubSystems(const char* szModuleName);

  /// \brief Output info about all known subsystems via the logging system (can change when DLLs are loaded dynamically).
  static void PrintAllSubsystems();

private:
  static void ComputeOrder(ezDeque<ezSubSystemDeclarationBase*>& Order);
  static bool HasDependencyOnModule(ezSubSystemDeclarationBase* pSubSystem, const char* szModule);

  static void Startup(ezStartupStage::Enum stage);
  static void Shutdown(ezStartupStage::Enum stage);

  static bool s_bPrintAllSubSystems;
};

