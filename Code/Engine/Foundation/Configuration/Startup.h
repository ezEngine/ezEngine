#pragma once

#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Configuration/Plugin.h>

#define EZ_GLOBALEVENT_STARTUP_CORE_BEGIN "ezStartup_StartupCore_Begin"
#define EZ_GLOBALEVENT_STARTUP_CORE_END "ezStartup_StartupCore_End"
#define EZ_GLOBALEVENT_SHUTDOWN_CORE_BEGIN "ezStartup_ShutdownCore_Begin"
#define EZ_GLOBALEVENT_SHUTDOWN_CORE_END "ezStartup_ShutdownCore_End"

#define EZ_GLOBALEVENT_STARTUP_ENGINE_BEGIN "ezStartup_StartupEngine_Begin"
#define EZ_GLOBALEVENT_STARTUP_ENGINE_END "ezStartup_StartupEngine_End"
#define EZ_GLOBALEVENT_SHUTDOWN_ENGINE_BEGIN "ezStartup_ShutdownEngine_Begin"
#define EZ_GLOBALEVENT_SHUTDOWN_ENGINE_END "ezStartup_ShutdownEngine_End"

#define EZ_GLOBALEVENT_UNLOAD_PLUGIN_BEGIN "ezStartup_UnloadPlugin_Begin"
#define EZ_GLOBALEVENT_UNLOAD_PLUGIN_END "ezStartup_UnloadPlugin_End"

/// \brief The startup system makes sure to initialize and shut down all known subsystems in the proper order.
///
/// Each subsystem can define on which other subsystems (or entire group) it is dependent (i.e. which other code it needs in an initialized
/// state, before it can run itself). The startup system will sort all subsystems by their dependencies and then initialize
/// them in the proper order.
/// The startup and shutdown sequence consists of two steps. First the 'core' functionality is initialized. This is usually
/// all the functionality that does not depend on a working rendering context.
/// After all systems have had their 'core' functionality initialized, the 'engine' functionality can be initialized.
/// In between these steps the rendering context should be created.
/// Tools that might not create a window or do not want to actually load GPU resources, might get away with only doing
/// the 'core' initialization. Thus a subsystem should do all the initialization that is independent from a window or rendering
/// context in 'core' startup, and it should be able to work (with some features disabled), even when 'engine' startup is not done.
///
/// A subsystem startup configuration for a static subsystem needs to be put in some cpp file of the subsystem and looks like this:
///
/// EZ_BEGIN_SUBSYSTEM_DECLARATION(ExampleGroup, ExampleSubSystem)
/// 
///   BEGIN_SUBSYSTEM_DEPENDENCIES
///     "SomeOtherSubSystem",
///     "SomeOtherSubSystem2",
///     "SomeGroup"
///   END_SUBSYSTEM_DEPENDENCIES
/// 
///   ON_CORE_STARTUP
///   {
///     ezExampleSubSystem::BasicStartup();
///   }
/// 
///   ON_CORE_SHUTDOWN
///   {
///     ezExampleSubSystem::BasicShutdown();
///   }
/// 
///   ON_ENGINE_STARTUP
///   {
///     ezExampleSubSystem::EngineStartup();
///   }
/// 
///   ON_ENGINE_SHUTDOWN
///   {
///     ezExampleSubSystem::EngineShutdown();
///   }
/// 
/// EZ_END_SUBSYSTEM_DECLARATION
///
/// This will automatically register the subsystem, once the code is being loaded (can be dynamically loaded from a DLL).
/// The next time any of the ezStartup functions are called (StartupCore, StartupEngine) the subsystem will be initialized.
///
/// If however your subsystem is implemented as a normal class, you need to derive from the base class 'ezSubSystem' and
/// override the virtual functions. Then when you have an instance of that class and call ezStartup::StartupCore etc., that
/// instance will be properly initialized as well. However, you must ensure that the subsystem is properly shut down, before
/// its instance is destroyed. Also you should never have two instances of the same subsystem.
///
/// All startup / shutdown procedures broadcast global events before and after they execute.
class EZ_FOUNDATION_DLL ezStartup
{
public:

  // 'Base Startup' happens even before 'Core Startup', but only really low level stuff should  be done there
  // and those subsystems should not have any dependencies on each other.
  // 'Base Startup' is automatically done right before 'Core Startup'
  // There is actually no 'Base Shutdown', everything that is initialized in 'Base Startup' should not require
  // any explicit shutdown.

  /// \brief Runs the 'base' startup sequence of all subsystems in the proper order.
  ///
  /// Run this, if you only require very low level systems to be initialized. Otherwise prefer StartupCore.
  /// There is NO ShutdownBase, everything that gets initialized during the 'Base Startup' should not need any deinitialization.
  /// This function is automatically called by StartupCore, if it hasn't been called before already.
  static void StartupBase() { Startup(ezStartupStage::Base); }

  /// \brief Runs the 'core' startup sequence of all subsystems in the proper order.
  ///
  /// Run this BEFORE any window and graphics context have been created.
  /// Broadcasts the global event EZ_GLOBALEVENT_STARTUP_CORE_BEGIN and EZ_GLOBALEVENT_STARTUP_CORE_END
  static void StartupCore() { Startup(ezStartupStage::Core); }

  /// \brief Runs the 'core' shutdown sequence of all subsystems in the proper order (reversed startup order).
  ///
  /// Call this AFTER window and graphics context have been destroyed already, shortly before application exit.
  /// Makes sure that the 'engine' shutdown has been run first.
  /// Broadcasts the global event EZ_GLOBALEVENT_SHUTDOWN_CORE_BEGIN and EZ_GLOBALEVENT_SHUTDOWN_CORE_END
  static void ShutdownCore() { Shutdown(ezStartupStage::Core); }

  /// \brief Runs the 'engine' startup sequence of all subsystems in the proper order.
  ///
  /// Run this AFTER a window and graphics context have been created, such that anything that depends on that
  /// can now do its initialization.
  /// Makes sure that the 'core' initialization has been run first.
  /// Broadcasts the global event EZ_GLOBALEVENT_STARTUP_ENGINE_BEGIN and EZ_GLOBALEVENT_STARTUP_ENGINE_END
  static void StartupEngine() { Startup(ezStartupStage::Engine); }

  /// \brief Runs the 'core' shutdown sequence of all subsystems in the proper order (reversed startup order).
  ///
  /// Run this BEFORE the window and graphics context have been destroyed, such that code that requires those
  /// can do its deinitialization first.
  /// Broadcasts the global event EZ_GLOBALEVENT_SHUTDOWN_ENGINE_BEGIN and EZ_GLOBALEVENT_SHUTDOWN_ENGINE_END
  static void ShutdownEngine() { Shutdown(ezStartupStage::Engine); }

  /// \brief Output info about all known subsystems via the logging system (can change when DLLs are loaded dynamically).
  static void PrintAllSubsystems();

  /// \brief Calls StartupBase, StartupCore or StartupEngine again, depending on what was done last.
  ///
  /// This can be used to first unload plugins and reload them, and then reinit the engine to the state that it was in again.
  static void ReinitToCurrentState();

private:

  /// \brief Unloads all subsystems from the given plugin AND all subsystems that directly or indirectly depend on them.
  ///
  /// This can be used to shutdown all systems from certain DLLs before that DLL is unloaded (and possibly reloaded).
  /// Broadcasts the global event EZ_GLOBALEVENT_UNLOAD_PLUGIN_BEGIN and EZ_GLOBALEVENT_UNLOAD_PLUGIN_END and passes szPluginName in the first event parameter.
  static void UnloadPluginSubSystems(const char* szPluginName);
  
  static void PluginEventHandler(const ezPlugin::PluginEvent& EventData);
  static void AssignSubSystemPlugin(const char* szPluginName);

  static void ComputeOrder(ezDeque<ezSubSystem*>& Order);
  static bool HasDependencyOnPlugin(ezSubSystem* pSubSystem, const char* szModule);

  static void Startup(ezStartupStage::Enum stage);
  static void Shutdown(ezStartupStage::Enum stage);

  static bool s_bPrintAllSubSystems;
  static ezStartupStage::Enum s_CurrentState;
};

