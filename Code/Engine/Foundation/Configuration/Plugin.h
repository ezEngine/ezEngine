#pragma once

/// \file

#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Communication/Event.h>

/// \brief ezPlugin allows to manage all dynamically loadable plugins. Each plugin DLL must contain one global instance of ezPlugin.
///
/// Put a global instance of ezPlugin somewhere into the source of each dynamic plugin DLL. Certain code depends on such instances
/// to work correctly with dynamically loaded code. For example ezStartup allows to initialize and deinitialize code from
/// dynamic DLLs properly (and in the correct order), by listening to events from ezPlugin.
/// ezPlugin also provides static functions to load and unload DLLs.
class EZ_FOUNDATION_DLL ezPlugin : public ezEnumerable<ezPlugin>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezPlugin);

public:
  /// \brief Callback type for when a plugin has just been loaded (not yet initialized). bReloading is true, if the plugin is currently being reloaded.
  typedef void(*OnPluginLoadedFunction)(bool bReloading); // [tested]

  /// \brief Callback type for when a plugin will be unloaded (after all deinitializations). bReloading is true, if the plugin is currently being reloaded.
  typedef void(*OnPluginUnloadedFunction)(bool bReloading); // [tested]

  /// \brief Call this before loading / unloading several plugins in a row, to prevent unnecessary re-initializations.
  static void BeginPluginChanges();

  /// \brief Must be called to finish what BeginPluginChanges started.
  static void EndPluginChanges();

  /// \brief Creates a new plugin object. 
  ///
  /// \param bIsReloadable
  ///   If set to true, 'ReloadPlugins' will reload this plugin (if it was modified).
  /// \param OnLoadPlugin
  ///   Will be called right after the plugin is loaded, even before other code is notified of that the plugin is now loaded.
  /// \param OnUnloadPlugin
  ///   Will be called shortly before the DLL is finally unloaded. All other code has already been notified that the plugin is being unloaded.
  /// \param szPluginDependency1
  ///   Allows to specify other modules that this plugin depends on. These will be automatically loaded and unloaded together with this plugin.
  ezPlugin(bool bIsReloadable, OnPluginLoadedFunction OnLoadPlugin = nullptr, OnPluginUnloadedFunction OnUnloadPlugin = nullptr, 
    const char* szPluginDependency1 = nullptr, const char* szPluginDependency2 = nullptr, const char* szPluginDependency3 = nullptr, const char* szPluginDependency4 = nullptr, const char* szPluginDependency5 = nullptr);

  /// \brief Returns the name that was used to load the plugin from disk.
  const char* GetPluginName() const { return m_sLoadedFromFile.GetData(); } // [tested]

  /// \brief Returns whether this plugin supports hot-reloading.
  bool IsReloadable() const { return m_bIsReloadable; } // [tested]

  /// \brief Tries to load a DLL dynamically into the program.
  ///
  /// For every time a plugin is loaded via 'LoadPlugin' it should also get unloaded via 'UnloadPlugin',
  /// as ezPlugin counts these and only unloads a plugin once its reference count reaches zero.
  /// 
  /// EZ_SUCCESS is returned when the DLL is either successfully loaded or has already been loaded before.
  /// EZ_FAILURE is returned if the DLL cannot be located or it could not be loaded properly.
  static ezResult LoadPlugin(const char* szPluginFile); // [tested]

  /// \brief Tries to unload a previously loaded plugin.
  ///
  /// For every time a plugin is loaded via 'LoadPlugin' it should also get unloaded via 'UnloadPlugin',
  /// as ezPlugin counts these and only unloads a plugin once its reference count reaches zero.
  /// If a plugin is not unloaded, because its refcount has not yet reached zero, 'UnloadPlugin' still returns EZ_SUCCESS.
  /// 
  /// EZ_SUCCESS is returned when the DLL is either successfully unloaded are has already been unloaded before (or has even never been loaded before).
  /// EZ_FAILURE is returned if the DLL cannot be unloaded (at this time).
  static ezResult UnloadPlugin(const char* szPluginFile); // [tested]

  /// \brief Hot-reloads all plugins that are marked as reloadable.
  ///
  /// Returns failure or success depending on whether (un-)loading of any of the hot-reloadable plugins failed.
  /// Even if one fails, it still tries to reload ALL plugins.
  /// If a reloadable plugin does not exist (anymore), that plugin is not even tried to be reloaded.
  /// If a plugin can be unloaded but reloading fails, a backup of the previous version is used instead.
  /// In case that fails as well, the application will probably crash.
  /// EZ_FAILURE is returned if anything could not be reloaded as desired, independent of whether the system was able
  /// to recover from it. So 'failure' means that not all reloadable code has been updated.
  static ezResult ReloadPlugins(bool bForceReload = false); // [tested]

  /// \brief Tries to find an ezPlugin instance by the given name. Returns nullptr if there is no such plugin.
  /// Can be used to check whether a certain plugin is loaded.
  static ezPlugin* FindPluginByName(const char* szPluginName); // [tested]

  /// \brief The data that is broadcast whenever a plugin is (un-) loaded.
  struct PluginEvent
  {
    enum Type
    {
      BeforeLoading,          ///< Sent shortly before a new plugin is loaded
      AfterLoadingBeforeInit, ///< Sent immediately after a new plugin has been loaded, even before it is initialized (which might trigger loading of other plugins)
      AfterLoading,           ///< Sent after a new plugin has been loaded and initialized
      BeforeUnloading,        ///< Sent before a plugin is going to be unloaded
      AfterUnloading,         ///< Sent after a plugin has been unloaded
      BeforePluginChanges,    ///< Sent (once) before any (group) plugin changes (load/unload) are done.
      AfterPluginChanges,     ///< Sent (once) after all (group) plugin changes (unload/load/reload) are finished.
    };

    Type m_EventType;           ///< Which type of event this is.
    ezPlugin* m_pPluginObject;  ///< Which plugin object is affected. Only available in 'AfterLoading' and 'BeforeUnloading'.
    const char* m_szPluginFile; ///< The file name in which the plugin that is loaded or unloaded is located.
  };

  /// \brief Code that needs to be execute whenever a plugin is loaded or unloaded can register itself here to be notified of such events.
  static ezEvent<const PluginEvent&> s_PluginEvents;

  /// \brief Returns the n-th plugin that this one is dependent on, or nullptr if there is no further dependency.
  const char* GetPluginDependency(ezUInt8 uiDependency) const { return (uiDependency < 5) ? m_szPluginDependencies[uiDependency] : nullptr; }

  /// \brief Sets how many tries the system will do to find a free plugin file name.
  ///
  /// During plugin loading the system creates copies of the plugin DLLs for reloading. This only works if the system can find a
  /// file to write to. If too many instances of the engine are running, no such free file name might be found and plugin loading fails.
  /// This value specifies how often the system tries to find a free file. The default is 32.
  static void SetMaxParallelInstances(ezUInt32 uiMaxParallelInstances);

private:
  static void GetPluginPaths(const char* szPluginName, ezStringBuilder& sOldPath, ezStringBuilder& sNewPath, ezUInt8 uiFileNumber);

  const char* m_szPluginDependencies[5];
  ezString m_sLoadedFromFile;

  OnPluginLoadedFunction m_OnLoadPlugin;
  OnPluginUnloadedFunction m_OnUnloadPlugin;

  static ezResult UnloadPluginInternal(const char* szPlugin, bool bReloading);
  static ezResult LoadPluginInternal(const char* szPlugin, bool bLoadCopy, bool bReloading);
  static void SortPluginReloadOrder(ezHybridArray<ezString, 16>& PluginsToReload);

  void Initialize(bool bReloading);
  void Uninitialize(bool bReloading);

  bool m_bInitialized;
  bool m_bIsReloadable;

  static ezUInt32 m_uiMaxParallelInstances;
  static ezInt32 s_iPluginChangeRecursionCounter;
};


/// \brief Insert this into a common header file of a plugin that is typically loaded dynamically, to enable easy integration for static linking as well.
///
/// This macro will declare some dummy variable to ensure that a plugin is always referenced (and thus linked into an application),
/// if that header with the declaration in it has been included in an application.
/// This enables integrating a library which is typically loaded dynamically, statically into an application.
///
/// Note that once the header that contains this macro has been included, the static reference is always there and thus linking
/// to the library is inevitable. If the library should be loaded dynamically, the application shouldn't even include any of its headers.
///
/// This macro also ensures that a plugin DLL exports any symbols at all, which is necessary on Windows to have a .lib and .exp file generated,
/// which is in turn required to statically link against the library.
#define EZ_DYNAMIC_PLUGIN_DECLARATION(LINKAGE, Plugin)      \
                                                            \
LINKAGE void ezPluginHelper_##Plugin();                     \
                                                            \
class ezDynamicPluginHelper_##Plugin                        \
{                                                           \
public:                                                     \
  ezDynamicPluginHelper_##Plugin()                          \
  {                                                         \
    ezPluginHelper_##Plugin();                              \
  }                                                         \
};                                                          \
                                                            \
static ezDynamicPluginHelper_##Plugin ezPluginHelperVar     \

/// \brief The counter part to EZ_DYNAMIC_PLUGIN_DECLARATION. Must be put into some cpp file of a plugin.
#define EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(Plugin)            \
  void ezPluginHelper_##Plugin() { }

