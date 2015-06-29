#include <Foundation/PCH.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/ReloadableVariable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Configuration/Implementation/Win/Plugin_Win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  #include <Foundation/Configuration/Implementation/Posix/Plugin_Posix.h>
#else
  #error "Plugins not implemented on this Platform."
#endif

ezResult UnloadPluginModule(ezPluginModule& Module, const char* szPluginFile);
ezResult LoadPluginModule(const char* szFileToLoad, ezPluginModule& Module, const char* szPluginFile);

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezPlugin);

/// \internal Struct to hold some data about loaded plugins.
struct PluginData
{
  PluginData()
  {
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
ezEvent<const ezPlugin::PluginEvent&> ezPlugin::s_PluginEvents;


ezPlugin::ezPlugin(bool bIsReloadable, OnPluginLoadedFunction OnLoadPlugin, OnPluginUnloadedFunction OnUnloadPlugin, const char* szPluginDependency1, const char* szPluginDependency2, const char* szPluginDependency3, const char* szPluginDependency4, const char* szPluginDependency5)
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
    PluginEvent e;
    e.m_EventType = PluginEvent::BeforePluginChanges;
    e.m_pPluginObject = nullptr;
    e.m_szPluginFile = nullptr;
    s_PluginEvents.Broadcast(e);

    ezReloadableVariableBase::StoreVariables();
  }

  ++s_iPluginChangeRecursionCounter;
}

void ezPlugin::EndPluginChanges()
{
  --s_iPluginChangeRecursionCounter;

  if (s_iPluginChangeRecursionCounter == 0)
  {
    PluginEvent e;
    e.m_EventType = PluginEvent::AfterPluginChanges;
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
    PluginEvent e;
    e.m_EventType = PluginEvent::BeforeUnloading;
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
    PluginEvent e;
    e.m_EventType = PluginEvent::AfterUnloading;
    e.m_pPluginObject = nullptr;
    e.m_szPluginFile = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  ezLog::Success("Plugin '%s' is unloaded.", szPluginFile);

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
    ezLog::Error("The plugin '%s' does not exist.", szPluginFile);
    return EZ_FAILURE;
  }

  if (bLoadCopy)
  {
    // create a copy of the original plugin file
    for (uiFileNumber = 0; uiFileNumber < ezPlugin::m_uiMaxParallelInstances; ++uiFileNumber)
    {
      GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin, uiFileNumber);
      if (ezOSFile::CopyFile(sOldPlugin.GetData(), sNewPlugin.GetData()) == EZ_SUCCESS)
        goto success;
    }

    ezLog::Error("Could not copy the plugin file '%s' to '%s' (and all previous file numbers). Plugin MaxParallelInstances is set to %i.", sOldPlugin.GetData(), sNewPlugin.GetData(), ezPlugin::m_uiMaxParallelInstances);

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
    PluginEvent e;
    e.m_EventType = PluginEvent::BeforeLoading;
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
          PluginEvent e;
          e.m_EventType = PluginEvent::AfterLoadingBeforeInit;
          e.m_pPluginObject = pPlugin;
          e.m_szPluginFile = szPluginFile;
          s_PluginEvents.Broadcast(e);
        }

        pPlugin->Initialize(bReloading);

        // Broadcast Event: After loading plugin
        {
          PluginEvent e;
          e.m_EventType = PluginEvent::AfterLoading;
          e.m_pPluginObject = pPlugin;
          e.m_szPluginFile = szPluginFile;
          s_PluginEvents.Broadcast(e);
        }

        ++iNewPlugins;
      }
      
      pPlugin = pPlugin->GetNextInstance();
    }

    EZ_ASSERT_RELEASE(iNewPlugins == 1, "A plugin must contain exactly one instance of an ezPlugin. While loading plugin '%s' %i ezPlugin instances were found.", szPluginFile, iNewPlugins);
  }

  ezLog::Success("Plugin '%s' is loaded.", szPluginFile);
  EndPluginChanges();
  return EZ_SUCCESS;
}

ezResult ezPlugin::LoadPlugin(const char* szPluginFile)
{
  ezStringBuilder sPlugin = szPluginFile;

  EZ_LOG_BLOCK("Loading Plugin", szPluginFile);

  if (g_LoadedPlugins.Find(szPluginFile).IsValid())
  {
    g_LoadedPlugins[szPluginFile].m_iReferenceCount++;
    ezLog::Dev("Plugin '%s' already loaded.", szPluginFile);
    return EZ_SUCCESS;
  }

  ezLog::Dev("Plugin to load: \"%s\"", szPluginFile);
  g_LoadedPlugins[szPluginFile].m_iReferenceCount = 1;

  return LoadPluginInternal(szPluginFile, true, false);
}

ezResult ezPlugin::UnloadPlugin(const char* szPluginFile)
{
  EZ_LOG_BLOCK("Unloading Plugin", szPluginFile);

  if (!g_LoadedPlugins.Find(szPluginFile).IsValid())
  {
    ezLog::Dev("Plugin '%s' is not loaded.", szPluginFile);
    return EZ_SUCCESS;
  }

  g_LoadedPlugins[szPluginFile].m_iReferenceCount--;

  if (g_LoadedPlugins[szPluginFile].m_iReferenceCount > 0)
  {
    ezLog::Dev("Plugin '%s' is still referenced (RefCount: %i).", szPluginFile, g_LoadedPlugins[szPluginFile].m_iReferenceCount);
    return EZ_SUCCESS;
  }

  ezLog::Dev("Plugin to unload: \"%s\"", szPluginFile);
  UnloadPluginInternal(szPluginFile, false);

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
          ezLog::Warning("Plugin '%s' does not exist at the moment. Plugin will not be reloaded.", pPlugin->m_sLoadedFromFile.GetData());
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
                if (g_LoadedPlugins[pPlugin->m_sLoadedFromFile].m_LastModificationTime.IsEqual(stat.m_LastModificationTime, ezTimestamp::CompareMode::FileTime))
                {
                  ezLog::Dev("Plugin '%s' is not modified.", pPlugin->GetPluginName());
                  bModified = false;
                }
                else
                  ezLog::Info("Plugin '%s' is modified, reloading.", pPlugin->GetPluginName());
              }
            }

          #endif
        }

        if (bModified)
        {
          ezStringBuilder sBackup = sNewPlugin;
          sBackup.Append(".backup");

          EZ_VERIFY(ezOSFile::CopyFile(sNewPlugin.GetData(), sBackup.GetData()) == EZ_SUCCESS, "Could not create backup of plugin '%s'", pPlugin->GetPluginName());

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

      EZ_VERIFY (UnloadPluginInternal(PluginsToReload[iIndex].GetData(), true) == EZ_SUCCESS, "Could not unload plugin '%s'.", PluginsToReload[iIndex].GetData());
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

      if (LoadPluginInternal(PluginsToReload[i].GetData(), true, true) == EZ_FAILURE)
      {
        ezLog::Error("Loading of Plugin '%s' failed. Falling back to backup of previous version.", PluginsToReload[i].GetData());

        res = EZ_FAILURE;

        if (ezOSFile::CopyFile(sBackup.GetData(), sNewPlugin.GetData()) == EZ_SUCCESS)
        {
          // if we cannot reload a plugin (not even its backup), all we can do is crash with an error message
          // everything else would most probably result in crashes in very strange ways
          EZ_VERIFY (LoadPluginInternal(PluginsToReload[i].GetData(), false, true) == EZ_SUCCESS, "Could not reload backup of plugin '%s'", PluginsToReload[i].GetData());
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

