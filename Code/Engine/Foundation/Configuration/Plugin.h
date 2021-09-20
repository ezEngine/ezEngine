#pragma once

/// \file

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

class ezPlugin;

/// \brief The data that is broadcast whenever a plugin is (un-) loaded.
struct ezPluginEvent
{
  enum Type
  {
    BeforeLoading,          ///< Sent shortly before a new plugin is loaded
    AfterLoadingBeforeInit, ///< Sent immediately after a new plugin has been loaded, even before it is initialized (which might trigger loading of
                            ///< other plugins)
    AfterLoading,           ///< Sent after a new plugin has been loaded and initialized
    BeforeUnloading,        ///< Sent before a plugin is going to be unloaded
    StartupShutdown,        ///< Used by the startup system for automatic shutdown
    AfterStartupShutdown,
    AfterUnloading,      ///< Sent after a plugin has been unloaded
    BeforePluginChanges, ///< Sent (once) before any (group) plugin changes (load/unload) are done.
    AfterPluginChanges,  ///< Sent (once) after all (group) plugin changes (unload/load) are finished.
  };

  Type m_EventType;                       ///< Which type of event this is.
  const char* m_szPluginBinary = nullptr; ///< The file name in which the plugin that is loaded or unloaded is located.
};

struct ezPluginLoadFlags
{
  using StorageType = ezUInt8;

  enum Enum
  {
    LoadCopy = EZ_BIT(0),               ///<
    PluginIsOptional = EZ_BIT(1),       ///<
    NoPluginObjectExpected = EZ_BIT(2), ///<

    Default = 0,
  };

  struct Bits
  {
    StorageType LoadCopy : 1;
    StorageType PluginIsOptional : 1;
    StorageType NoPluginObjectExpected : 1;
  };
};

using ezPluginInitCallback = void (*)();

/// \brief ezPlugin allows to manage all dynamically loadable plugins. Each plugin DLL must contain one global instance of ezPlugin.
///
/// Put a global instance of ezPlugin somewhere into the source of each dynamic plugin DLL. Certain code depends on such instances
/// to work correctly with dynamically loaded code. For example ezStartup allows to initialize and deinitialize code from
/// dynamic DLLs properly (and in the correct order), by listening to events from ezPlugin.
/// ezPlugin also provides static functions to load and unload DLLs.
class EZ_FOUNDATION_DLL ezPlugin
{
public:
  /// \brief Call this before loading / unloading several plugins in a row, to prevent unnecessary re-initializations.
  static void BeginPluginChanges();

  /// \brief Must be called to finish what BeginPluginChanges started.
  static void EndPluginChanges();

  /// \brief Checks whether a plugin with the given name exists. Does not guarantee that the plugin could be loaded successfully.
  static bool ExistsPluginFile(const char* szPluginFile);

  /// \brief Tries to load a DLL dynamically into the program.
  ///
  /// For every time a plugin is loaded via 'LoadPlugin' it should also get unloaded via 'UnloadPlugin',
  /// as ezPlugin counts these and only unloads a plugin once its reference count reaches zero.
  ///
  /// EZ_SUCCESS is returned when the DLL is either successfully loaded or has already been loaded before.
  /// EZ_FAILURE is returned if the DLL cannot be located or it could not be loaded properly.
  static ezResult LoadPlugin(const char* szPluginFile, ezBitflags<ezPluginLoadFlags> flags = ezPluginLoadFlags::Default); // [tested]

  /// \brief Attempts to unload all previously loaded plugins in the reverse order in which they were loaded.
  static void UnloadAllPlugins();

  /// \brief Code that needs to be execute whenever a plugin is loaded or unloaded can register itself here to be notified of such events.
  static ezCopyOnBroadcastEvent<const ezPluginEvent&> s_PluginEvents;

  /// \brief Sets how many tries the system will do to find a free plugin file name.
  ///
  /// During plugin loading the system may create copies of the plugin DLLs. This only works if the system can find a
  /// file to write to. If too many instances of the engine are running, no such free file name might be found and plugin loading fails.
  /// This value specifies how often the system tries to find a free file. The default is 32.
  static void SetMaxParallelInstances(ezUInt32 uiMaxParallelInstances);

  /// \brief Returns the name of the binary through which the plugin was loaded.
  //const char* GetOriginBinary() const { return m_sOriginBinary; }

  static void InitializeStaticallyLinkedPlugins();

  struct EZ_FOUNDATION_DLL Init
  {
    Init(ezPluginInitCallback OnLoadOrUnloadCB, bool bOnLoad);
    Init(const char* szAddPluginDependency);
  };

private:
  ezPlugin() = delete;

  static void GetPluginPaths(const char* szPluginName, ezStringBuilder& sOldPath, ezStringBuilder& sNewPath, ezUInt8 uiFileNumber);

  static ezResult UnloadPluginInternal(const char* szPlugin);
  static ezResult LoadPluginInternal(const char* szPlugin, ezBitflags<ezPluginLoadFlags> flags);
};

#define EZ_BEGIN_PLUGIN(a)
#define EZ_END_PLUGIN
#define BEGIN_PLUGIN_DEPENDENCIES
#define END_PLUGIN_DEPENDENCIES

#define EZ_PLUGIN_DEPENDENCY(PluginName) \
  ezPlugin::Init EZ_CONCAT(EZ_CONCAT(plugin_dep_, PluginName), EZ_SOURCE_LINE)(EZ_PP_STRINGIFY(PluginName))

#define ON_PLUGIN_LOADED                                     \
  static void plugin_OnLoaded();                             \
  ezPlugin::Init plugin_OnLoadedInit(plugin_OnLoaded, true); \
  static void plugin_OnLoaded()

#define ON_PLUGIN_UNLOADED                                        \
  static void plugin_OnUnloaded();                                \
  ezPlugin::Init plugin_OnUnloadedInit(plugin_OnUnloaded, false); \
  static void plugin_OnUnloaded()
