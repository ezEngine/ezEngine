#pragma once

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
  /// \brief Callback type for when a plugin has just been loaded (not yet initialized).
  typedef void(*OnPluginLoadedFunction)();

  /// \brief Callback type for when a plugin will be unloaded (after all deinitializations).
  typedef void(*OnPluginUnloadedFunction)();

  /// \brief Call this before loading / unloading several plugins in a row, to prevent unnecessary re-initializations.
  static void BeginPluginChanges();

  /// \brief Must be called to finish what BeginPluginChanges started.
  static void EndPluginChanges();

  /// \brief Creates a new plugin object. 
  ///
  /// \param szPluginName
  ///   The name of this plugin. Should be unique across all plugins. 
  /// \param bIsReloadable
  ///   If set to true, 'ReloadPlugins' will reload this plugin (if it was modified).
  /// \param OnLoadPlugin
  ///   Will be called right after the plugin is loaded, even before other code is notified of that the plugin is now loaded.
  /// \param OnUnloadPlugin
  ///   Will be called shortly before the DLL is finally unloaded. All other code has already been notified that the plugin is being unloaded.
  /// \param szPluginDependency1
  ///   Allows to specify other modules that this plugin depends on. These will be automatically loaded and unloaded together with this plugin.
  ezPlugin(const char* szPluginName, bool bIsReloadable, OnPluginLoadedFunction OnLoadPlugin = NULL, OnPluginUnloadedFunction OnUnloadPlugin = NULL, 
    const char* szPluginDependency1 = NULL, const char* szPluginDependency2 = NULL, const char* szPluginDependency3 = NULL, const char* szPluginDependency4 = NULL, const char* szPluginDependency5 = NULL);

  /// \brief Returns the internal name of the plugin. Might differ from the filename from which it is loaded.
  const char* GetPluginName() const { return m_szPluginName; }

  /// \brief Returns the name that was used to load the plugin from disk.
  const char* GetPluginFileName() const { return m_sLoadedFromFile.GetData(); }

  /// \brief Returns whether this plugin supports hot-reloading.
  bool IsReloadable() const { return m_bIsReloadable; }

  /// \brief Tries to load a DLL dynamically into the program.
  ///
  /// For every time a plugin is loaded via 'LoadPlugin' it should also get unloaded via 'UnloadPlugin',
  /// as ezPlugin counts these and only unloads a plugin once its reference count reaches zero.
  /// 
  /// EZ_SUCCESS is returned when the DLL is either successfully loaded or has already been loaded before.
  /// EZ_FAILURE is returned if the DLL cannot be located or it could not be loaded properly.
  static ezResult LoadPlugin(const char* szPluginFile);

  /// \brief Tries to unload a previously loaded plugin.
  ///
  /// For every time a plugin is loaded via 'LoadPlugin' it should also get unloaded via 'UnloadPlugin',
  /// as ezPlugin counts these and only unloads a plugin once its reference count reaches zero.
  /// If a plugin is not unloaded, because its refcount has not yet reached zero, 'UnloadPlugin' still returns EZ_SUCCESS.
  /// 
  /// EZ_SUCCESS is returned when the DLL is either successfully unloaded are has already been unloaded before (or has even never been loaded before).
  /// EZ_FAILURE is returned if the DLL cannot be unloaded (at this time).
  static ezResult UnloadPlugin(const char* szPluginFile);

  /// \brief Hot-reloads all plugins that are marked as reloadable.
  ///
  /// Returns failure or success depending on whether (un-)loading of any of the hot-reloadable plugins failed.
  /// Even if one fails, it still tries to reload ALL plugins.
  /// If a reloadable plugin does not exist (anymore), that plugin is not even tried to be reloaded.
  /// If a plugin can be unloaded but reloading fails, a backup of the previous version is used instead.
  /// In case that fails as well, the application will probably crash.
  /// EZ_FAILURE is returned if anything could not be reloaded as desired, independent of whether the system was able
  /// to recover from it. So 'failure' means that not all reloadable code has been updated.
  static ezResult ReloadPlugins();

  /// \brief The data that is broadcasted whenever a plugin is (un-) loaded.
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
  static ezEvent<const PluginEvent&, void*, ezStaticAllocatorWrapper> s_PluginEvents;

private:
  static void GetPluginPaths(const char* szPluginName, ezStringBuilder& sOldPath, ezStringBuilder& sNewPath);

  const char* m_szPluginDependencies[5];
  const char* m_szPluginName;
  ezString m_sLoadedFromFile;

  OnPluginLoadedFunction m_OnLoadPlugin;
  OnPluginUnloadedFunction m_OnUnloadPlugin;

  static ezResult UnloadPluginInternal(const char* szPlugin);
  static ezResult LoadPluginInternal(const char* szPlugin, bool bLoadCopy);

  void Initialize();
  void Uninitialize();

  bool m_bInitialized;
  bool m_bIsReloadable;

  static ezInt32 s_iPluginChangeRecursionCounter;
};

