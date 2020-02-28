#include <FoundationPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Configuration/Implementation/Win/Plugin_Win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Configuration/Implementation/Posix/Plugin_Posix.h>
#else
#  error "Plugins not implemented on this Platform."
#endif

ezResult UnloadPluginModule(ezPluginModule& Module, const char* szPluginFile);
ezResult LoadPluginModule(const char* szFileToLoad, ezPluginModule& Module, const char* szPluginFile);

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezPlugin);

/// \internal Struct to hold some data about loaded plugins.
struct PluginData
{
  PluginData()
  {
    m_hModule = 0;
    m_pPluginObject = nullptr;
    m_iReferenceCount = 0;
    m_LastModificationTime.Invalidate();
    m_uiFileNumber = 0;
  }

  ezPluginModule m_hModule;
  ezUInt8 m_uiFileNumber;
  ezTimestamp m_LastModificationTime;
  ezPlugin* m_pPluginObject;
  ezInt32 m_iReferenceCount;
};

static ezMap<ezString, PluginData> g_LoadedPlugins;
ezInt32 ezPlugin::s_iPluginChangeRecursionCounter = 0;
ezUInt32 ezPlugin::m_uiMaxParallelInstances = 32;
ezEvent<const ezPluginEvent&> ezPlugin::s_PluginEvents;


void ezPlugin::SetMaxParallelInstances(ezUInt32 uiMaxParallelInstances)
{
  m_uiMaxParallelInstances = ezMath::Max(1u, uiMaxParallelInstances);
}

ezPlugin::ezPlugin(bool bIsReloadable, OnPluginLoadedFunction OnLoadPlugin, OnPluginUnloadedFunction OnUnloadPlugin,
  const char* szPluginDependency1, const char* szPluginDependency2, const char* szPluginDependency3,
  const char* szPluginDependency4, const char* szPluginDependency5)
{
  m_bInitialized = false;
  m_OnLoadPlugin = OnLoadPlugin;
  m_OnUnloadPlugin = OnUnloadPlugin;
  m_bIsReloadable = bIsReloadable;

  m_szPluginDependencies[0] = szPluginDependency1;
  m_szPluginDependencies[1] = szPluginDependency2;
  m_szPluginDependencies[2] = szPluginDependency3;
  m_szPluginDependencies[3] = szPluginDependency4;
  m_szPluginDependencies[4] = szPluginDependency5;
}

void ezPlugin::Initialize(bool bReloading)
{
  if (m_bInitialized)
    return;

  m_bInitialized = true;

  // do not load dependencies while reloading
  // the dependencies are already loaded, and this would only mess up
  // the reference count
  if (!bReloading)
  {
    for (ezInt32 i = 0; i < 5; ++i)
    {
      if (m_szPluginDependencies[i])
        LoadPlugin(m_szPluginDependencies[i]);
    }
  }

  if (m_OnLoadPlugin)
    m_OnLoadPlugin(bReloading);
}

void ezPlugin::Uninitialize(bool bReloading)
{
  if (!m_bInitialized)
    return;

  m_bInitialized = false;

  if (m_OnUnloadPlugin)
    m_OnUnloadPlugin(bReloading);

  // do not unload dependencies while reloading
  // the dependencies will be reloaded, if they are marked as reloadable
  // if we were to unload them here, ALL dependencies would get reloaded
  if (!bReloading)
  {
    for (ezInt32 i = 5; i > 0; --i)
    {
      if (m_szPluginDependencies[i - 1])
        UnloadPlugin(m_szPluginDependencies[i - 1]);
    }
  }
}

void ezPlugin::BeginPluginChanges()
{
  if (s_iPluginChangeRecursionCounter == 0)
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::BeforePluginChanges;
    e.m_pPluginObject = nullptr;
    e.m_szPluginFile = nullptr;
    s_PluginEvents.Broadcast(e);
  }

  ++s_iPluginChangeRecursionCounter;
}

void ezPlugin::EndPluginChanges()
{
  --s_iPluginChangeRecursionCounter;

  if (s_iPluginChangeRecursionCounter == 0)
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::AfterPluginChanges;
    e.m_pPluginObject = nullptr;
    e.m_szPluginFile = nullptr;
    s_PluginEvents.Broadcast(e);
  }
}

ezResult ezPlugin::UnloadPluginInternal(const char* szPluginFile, bool bReloading)
{
  BeginPluginChanges();

  if (!g_LoadedPlugins.Find(szPluginFile).IsValid())
    return EZ_SUCCESS;

  // Broadcast event: Before unloading plugin
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::BeforeUnloading;
    e.m_pPluginObject = g_LoadedPlugins[szPluginFile].m_pPluginObject;
    e.m_szPluginFile = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: Startup Shutdown
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::StartupShutdown;
    e.m_pPluginObject = g_LoadedPlugins[szPluginFile].m_pPluginObject;
    e.m_szPluginFile = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: After Startup Shutdown
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::AfterStartupShutdown;
    e.m_pPluginObject = g_LoadedPlugins[szPluginFile].m_pPluginObject;
    e.m_szPluginFile = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // if there is a plugin object, uninitialize it
  if (g_LoadedPlugins[szPluginFile].m_pPluginObject)
  {
    g_LoadedPlugins[szPluginFile].m_pPluginObject->Uninitialize(bReloading);
  }

  // unload the plugin module
  if (UnloadPluginModule(g_LoadedPlugins[szPluginFile].m_hModule, szPluginFile) == EZ_FAILURE)
  {
    EndPluginChanges();
    return EZ_FAILURE;
  }

  // delete the plugin copy that we had loaded
  {
    ezStringBuilder sOldPlugin, sNewPlugin;
    GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin, g_LoadedPlugins[szPluginFile].m_uiFileNumber);

    ezOSFile::DeleteFile(sNewPlugin.GetData());
  }

  // if the refcount is zero (i.e. we are not 'reloading' plugins), remove the info about the plugin
  if (g_LoadedPlugins[szPluginFile].m_iReferenceCount == 0)
    g_LoadedPlugins.Remove(szPluginFile);

  // Broadcast event: After unloading plugin
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::AfterUnloading;
    e.m_pPluginObject = nullptr;
    e.m_szPluginFile = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  ezLog::Success("Plugin '{0}' is unloaded.", szPluginFile);

  EndPluginChanges();

  return EZ_SUCCESS;
}

ezResult ezPlugin::LoadPluginInternal(const char* szPluginFile, bool bLoadCopy, bool bReloading)
{
  ezUInt8 uiFileNumber = 0;

  ezStringBuilder sOldPlugin, sNewPlugin;
  GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin, uiFileNumber);

  if (!ezOSFile::ExistsFile(sOldPlugin))
  {
    ezLog::Error("The plugin '{0}' does not exist.", szPluginFile);
    return EZ_FAILURE;
  }

  if (bLoadCopy)
  {
    // create a copy of the original plugin file
    const ezUInt8 uiMaxParallelInstances = static_cast<ezUInt8>(ezPlugin::m_uiMaxParallelInstances);
    for (uiFileNumber = 0; uiFileNumber < uiMaxParallelInstances; ++uiFileNumber)
    {
      GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin, uiFileNumber);
      if (ezOSFile::CopyFile(sOldPlugin.GetData(), sNewPlugin.GetData()) == EZ_SUCCESS)
        goto success;
    }

    ezLog::Error(
      "Could not copy the plugin file '{0}' to '{1}' (and all previous file numbers). Plugin MaxParallelInstances is set to {2}.",
      sOldPlugin, sNewPlugin, ezPlugin::m_uiMaxParallelInstances);

    g_LoadedPlugins.Remove(sNewPlugin);
    return EZ_FAILURE;
  }
  else
  {
    sNewPlugin = sOldPlugin;
  }

success:

  g_LoadedPlugins[szPluginFile].m_uiFileNumber = uiFileNumber;

  BeginPluginChanges();

  // Broadcast Event: Before loading plugin
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::BeforeLoading;
    e.m_pPluginObject = nullptr;
    e.m_szPluginFile = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  if (LoadPluginModule(sNewPlugin.GetData(), g_LoadedPlugins[szPluginFile].m_hModule, szPluginFile) == EZ_FAILURE)
  {
    g_LoadedPlugins.Remove(szPluginFile);
    EndPluginChanges();

    return EZ_FAILURE;
  }

// if the platform supports it, store the modification time of the plugin, so that we don't need to reload it, if not necessary
#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  {
    ezFileStats stat;
    if (ezOSFile::GetFileStats(sOldPlugin.GetData(), stat) == EZ_SUCCESS)
    {
      g_LoadedPlugins[szPluginFile].m_LastModificationTime = stat.m_LastModificationTime;
    }
  }
#endif


  // Find all known plugin objects
  {
    ezPlugin* pPlugin = ezPlugin::GetFirstInstance();

    ezInt32 iNewPlugins = 0;

    while (pPlugin)
    {
      if (!pPlugin->m_bInitialized)
      {
        g_LoadedPlugins[szPluginFile].m_pPluginObject = pPlugin;
        pPlugin->m_sLoadedFromFile = szPluginFile;

        // Broadcast Event: After loading plugin, before init
        {
          ezPluginEvent e;
          e.m_EventType = ezPluginEvent::AfterLoadingBeforeInit;
          e.m_pPluginObject = pPlugin;
          e.m_szPluginFile = szPluginFile;
          s_PluginEvents.Broadcast(e);
        }

        pPlugin->Initialize(bReloading);

        // Broadcast Event: After loading plugin
        {
          ezPluginEvent e;
          e.m_EventType = ezPluginEvent::AfterLoading;
          e.m_pPluginObject = pPlugin;
          e.m_szPluginFile = szPluginFile;
          s_PluginEvents.Broadcast(e);
        }

        ++iNewPlugins;
      }

      pPlugin = pPlugin->GetNextInstance();
    }

    /// \todo this doesn't work if one dynamic (e.g. editor) plugin has a link dependency on another dynamic (runtime) plugin
    // EZ_ASSERT_RELEASE(iNewPlugins == 1, "A plugin must contain exactly one instance of an ezPlugin. While loading plugin '{0}' {1}
    // ezPlugin instances were found.", szPluginFile, iNewPlugins);
  }

  ezLog::Success("Plugin '{0}' is loaded.", szPluginFile);
  EndPluginChanges();
  return EZ_SUCCESS;
}

bool ezPlugin::ExistsPluginFile(const char* szPluginFile)
{
  ezStringBuilder sOldPlugin, sNewPlugin;
  GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin, 0);

  return ezOSFile::ExistsFile(sOldPlugin);
}

ezResult ezPlugin::LoadPlugin(const char* szPluginFile, bool bLoadCopy /*= false*/)
{
  ezStringBuilder sPlugin = szPluginFile;

  EZ_LOG_BLOCK("Loading Plugin", szPluginFile);

  if (g_LoadedPlugins.Find(szPluginFile).IsValid())
  {
    g_LoadedPlugins[szPluginFile].m_iReferenceCount++;
    ezLog::Debug("Plugin '{0}' already loaded.", szPluginFile);
    return EZ_SUCCESS;
  }

  ezLog::Debug("Plugin to load: \"{0}\"", szPluginFile);

  ezResult res = LoadPluginInternal(szPluginFile, bLoadCopy, false);

  if (res.Succeeded())
  {
    if (g_LoadedPlugins[szPluginFile].m_iReferenceCount != 0)
      ezLog::Warning("Plugin '{0}' seems to have a circular dependency on itself.", szPluginFile);

    g_LoadedPlugins[szPluginFile].m_iReferenceCount++;
  }

  return res;
}

ezResult ezPlugin::LoadOptionalPlugin(const char* szPluginFile, bool bLoadCopy /*= false*/)
{
  if (!ExistsPluginFile(szPluginFile))
    return EZ_FAILURE;

  return LoadPlugin(szPluginFile, bLoadCopy);
}

ezResult ezPlugin::UnloadPlugin(const char* szPluginFile, ezInt32* out_pCurRefCount /*= nullptr*/)
{
  EZ_LOG_BLOCK("Unloading Plugin", szPluginFile);

  if (out_pCurRefCount)
    *out_pCurRefCount = 0;

  if (!g_LoadedPlugins.Find(szPluginFile).IsValid())
  {
    ezLog::Debug("Plugin '{0}' is not loaded.", szPluginFile);
    return EZ_SUCCESS;
  }

  EZ_ASSERT_DEBUG(g_LoadedPlugins[szPluginFile].m_iReferenceCount > 0, "Incorrect reference count");
  g_LoadedPlugins[szPluginFile].m_iReferenceCount--;

  if (out_pCurRefCount)
    *out_pCurRefCount = g_LoadedPlugins[szPluginFile].m_iReferenceCount;

  if (g_LoadedPlugins[szPluginFile].m_iReferenceCount > 0)
  {
    ezLog::Debug("Plugin '{0}' is still referenced (RefCount: {1}).", szPluginFile, g_LoadedPlugins[szPluginFile].m_iReferenceCount);
    return EZ_SUCCESS;
  }

  ezLog::Debug("Plugin to unload: \"{0}\"", szPluginFile);
  UnloadPluginInternal(szPluginFile, false);

  return EZ_SUCCESS;
}

ezResult ezPlugin::UnloadAllPlugins()
{
  ezHybridArray<ezString, 16> ToUnload;

  // if a plugin is linked statically (which happens mostly in an editor context)
  // then it cannot be unloaded and the ezPlugin instance won't ever go away
  // however, ezPlugin::UnloadPlugin will always return that it is already unloaded, so we can just skip it there
  // all other plugins must be unloaded as often as their refcount, though

  // also, all plugins must be unloaded in the reverse order in which they were originally loaded
  // otherwise a plugin may crash during its shutdown, because a dependency was already shutdown before it
  // fortunately the loading order is recorded in the ezPlugin instance chain and we just need to traverse it backwards
  // this happens especially when you load plugin A, and then plugin B, which itself has a fixed link dependency on A (not dynamically loaded)
  // and thus needs A during its shutdown
  ezStringBuilder s;
  ezPlugin* pPlugin = ezPlugin::GetFirstInstance();
  while (pPlugin != nullptr)
  {
    s = pPlugin->GetPluginName();
    ToUnload.PushBack(s);

    pPlugin = pPlugin->GetNextInstance();
  }

  ezString temp;
  while (!ToUnload.IsEmpty())
  {
    auto it = ToUnload.PeekBack();

    ezInt32 iRefCount = 0;
    if (ezPlugin::UnloadPlugin(it, &iRefCount).Failed())
    {
      ezLog::Error("Failed to unload plugin '{0}'", s);
      return EZ_FAILURE;
    }

    if (iRefCount == 0)
      ToUnload.PopBack();
  }

  return EZ_SUCCESS;
}

ezPlugin* ezPlugin::FindPluginByName(const char* szPluginName)
{
  ezPlugin* pPlugin = ezPlugin::GetFirstInstance();

  while (pPlugin)
  {
    if (ezStringUtils::IsEqual(szPluginName, pPlugin->GetPluginName()))
      return pPlugin;

    pPlugin = pPlugin->GetNextInstance();
  }

  return nullptr;
}

void ezPlugin::SortPluginReloadOrder(ezHybridArray<ezString, 16>& PluginsToReload)
{
  EZ_LOG_BLOCK("SortPluginReloadOrder");

  // sorts all plugins by their plugin-dependencies, such that the first item in the array does not depend
  // on any of the other plugins in the list and the successive items only depend on previous items in the list

  ezHybridArray<ezPlugin*, 16> PluginsToSort;
  ezSet<ezString> NotYetSorted;

  // convert the plugin names to pointers
  for (ezUInt32 i = 0; i < PluginsToReload.GetCount(); ++i)
  {
    NotYetSorted.Insert(PluginsToReload[i]);
    PluginsToSort.PushBack(FindPluginByName(PluginsToReload[i].GetData()));
  }

  // we will recreate that array in sorted order
  PluginsToReload.Clear();

  while (!NotYetSorted.IsEmpty()) // that's like three negations in one line...
  {
    bool bFoundAny = false;

    // find the next plugin that has no dependency anymore
    for (ezUInt32 iPlugin = 0; iPlugin < PluginsToSort.GetCount(); ++iPlugin)
    {
      if (PluginsToSort[iPlugin] == nullptr) // plugins that have been inserted are removed this way from the array
        continue;

      bool bHasDependency = false;

      // check if any of the plugin dependencies is still in the NotYetSorted set
      for (ezUInt32 iDep = 0; iDep < 5; ++iDep)
      {
        if (NotYetSorted.Find(PluginsToSort[iPlugin]->m_szPluginDependencies[iDep]).IsValid())
        {
          // The plugin has a dependency on another plugin that is not yet in the list -> do not put it into the list yet
          bHasDependency = true;
          break;
        }
      }

      // this plugin has no dependency, so remove it from the NotYetSorted list and append it to the output array
      if (!bHasDependency)
      {
        PluginsToReload.PushBack(PluginsToSort[iPlugin]->GetPluginName());
        NotYetSorted.Remove(PluginsToSort[iPlugin]->GetPluginName());
        PluginsToSort[iPlugin] = nullptr;

        bFoundAny = true;
      }
    }

    EZ_VERIFY(bFoundAny, "The reloadable plugins seem to have circular dependencies.");
  }
}

ezResult ezPlugin::ReloadPlugins(bool bForceReload)
{
  EZ_REPORT_FAILURE("This currently cannot work, because plugins do not load copied plugins.");

  EZ_LOG_BLOCK("Reload Plugins");

  ezLog::Dev("Reloading Plugins");

  ezHybridArray<ezString, 16> PluginsToReload;
  ezResult res = EZ_SUCCESS;

  BeginPluginChanges();

  // find all plugins to reload and unload them
  {
    ezPlugin* pPlugin = ezPlugin::GetFirstInstance();

    while (pPlugin)
    {
      if (pPlugin->IsReloadable())
      {
        bool bModified = true;

        ezStringBuilder sOldPlugin, sNewPlugin;
        GetPluginPaths(pPlugin->m_sLoadedFromFile, sOldPlugin, sNewPlugin, g_LoadedPlugins[pPlugin->m_sLoadedFromFile].m_uiFileNumber);

        if (!ezOSFile::ExistsFile(sOldPlugin.GetData()))
        {
          bModified = false;
          res = EZ_FAILURE;
          ezLog::Warning("Plugin '{0}' does not exist at the moment. Plugin will not be reloaded.", pPlugin->m_sLoadedFromFile);
        }
        else
        {
// if the platform supports file stat calls, we can check whether the plugin has been modified
// otherwise we always reload it (same as bForceReload)
#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)

          if (!bForceReload)
          {
            ezFileStats stat;
            if (ezOSFile::GetFileStats(sOldPlugin.GetData(), stat) == EZ_SUCCESS)
            {
              if (g_LoadedPlugins[pPlugin->m_sLoadedFromFile].m_LastModificationTime.Compare(stat.m_LastModificationTime,
                    ezTimestamp::CompareMode::FileTimeEqual))
              {
                ezLog::Debug("Plugin '{0}' is not modified.", pPlugin->GetPluginName());
                bModified = false;
              }
              else
                ezLog::Info("Plugin '{0}' is modified, reloading.", pPlugin->GetPluginName());
            }
          }

#endif
        }

        if (bModified)
        {
          ezStringBuilder sBackup = sNewPlugin;
          sBackup.Append(".backup");

          EZ_VERIFY(ezOSFile::CopyFile(sNewPlugin.GetData(), sBackup.GetData()) == EZ_SUCCESS, "Could not create backup of plugin '{0}'",
            pPlugin->GetPluginName());

          PluginsToReload.PushBack(pPlugin->m_sLoadedFromFile);
        }
      }

      pPlugin = pPlugin->GetNextInstance();
    }
  }

  // sort plugins by their dependencies, such that no plugin gets unloaded
  // before all plugins that might depend on it have been unloaded
  SortPluginReloadOrder(PluginsToReload);

  // now unload all modified plugins
  {
    EZ_LOG_BLOCK("Unloading Plugins");

    for (ezUInt32 i = PluginsToReload.GetCount(); i > 0; --i)
    {
      ezUInt32 iIndex = i - 1;

      EZ_VERIFY(UnloadPluginInternal(PluginsToReload[iIndex].GetData(), true) == EZ_SUCCESS, "Could not unload plugin '{0}'.",
        PluginsToReload[iIndex]);
    }
  }

  // now load all unloaded plugins (reverse unloading order)
  {
    EZ_LOG_BLOCK("Loading Plugins");

    for (ezUInt32 i = 0; i < PluginsToReload.GetCount(); ++i)
    {
      ezStringBuilder sOldPlugin, sNewPlugin;
      GetPluginPaths(PluginsToReload[i].GetData(), sOldPlugin, sNewPlugin, g_LoadedPlugins[PluginsToReload[i]].m_uiFileNumber);

      ezStringBuilder sBackup = sNewPlugin;
      sBackup.Append(".backup");

      /// \todo Set "copy dll" back to true again, when we know which plugins may be copied
      if (LoadPluginInternal(PluginsToReload[i].GetData(), false, true) == EZ_FAILURE)
      {
        ezLog::Error("Loading of Plugin '{0}' failed. Falling back to backup of previous version.", PluginsToReload[i]);

        res = EZ_FAILURE;

        if (ezOSFile::CopyFile(sBackup.GetData(), sNewPlugin.GetData()) == EZ_SUCCESS)
        {
          // if we cannot reload a plugin (not even its backup), all we can do is crash with an error message
          // everything else would most probably result in crashes in very strange ways
          EZ_VERIFY(LoadPluginInternal(PluginsToReload[i].GetData(), false, true) == EZ_SUCCESS, "Could not reload backup of plugin '{0}'",
            PluginsToReload[i]);
        }
      }

      // delete the backup file again
      ezOSFile::DeleteFile(sBackup.GetData());
    }
  }

  EndPluginChanges();

  return res;
}


EZ_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Plugin);
