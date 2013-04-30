#include <Foundation/PCH.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/OSFile.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Configuration/Implementation/Win/Plugin_Win.h>
#else
  #error "Unknown Platform."
#endif

ezResult UnloadPluginModule(ezPluginModule& Module, const char* szPluginFile);
ezResult LoadPluginModule(const char* szFileToLoad, ezPluginModule& Module, const char* szPluginFile);

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezPlugin);

/// \internal Struct to hold some data about loaded plugins.
struct PluginData
{
  PluginData()
  {
    m_pPluginObject = NULL;
    m_iReferenceCount = 0;
    m_uiLastModificationTime = 0;
  }

  ezPluginModule m_hModule;
  ezUInt64 m_uiLastModificationTime;
  ezPlugin* m_pPluginObject;
  ezInt32 m_iReferenceCount;
};

static ezMap<ezString, PluginData, ezCompareHelper<ezString>, ezStaticAllocatorWrapper> g_LoadedPlugins;
ezInt32 ezPlugin::s_iPluginChangeRecursionCounter = 0;
ezEvent<const ezPlugin::PluginEvent&, void*, ezStaticAllocatorWrapper> ezPlugin::s_PluginEvents;


ezPlugin::ezPlugin(const char* szPluginName, bool bIsReloadable, OnPluginLoadedFunction OnLoadPlugin, OnPluginUnloadedFunction OnUnloadPlugin, const char* szPluginDependency1, const char* szPluginDependency2, const char* szPluginDependency3, const char* szPluginDependency4, const char* szPluginDependency5)
{
  m_szPluginName = szPluginName;
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

void ezPlugin::Initialize()
{
  if (m_bInitialized)
    return;

  m_bInitialized = true;

  for (ezInt32 i = 0; i < 5; ++i)
  {
    if (m_szPluginDependencies[i])
      LoadPlugin(m_szPluginDependencies[i]);
  }

  if (m_OnLoadPlugin)
    m_OnLoadPlugin();
}

void ezPlugin::Uninitialize()
{
  if (!m_bInitialized)
    return;

  m_bInitialized = false;

  if (m_OnUnloadPlugin)
    m_OnUnloadPlugin();

  for (ezInt32 i = 5; i > 0; --i)
  {
    if (m_szPluginDependencies[i - 1])
      UnloadPlugin(m_szPluginDependencies[i - 1]);
  }
}

void ezPlugin::BeginPluginChanges()
{
  if (s_iPluginChangeRecursionCounter == 0)
  {
    PluginEvent e;
    e.m_EventType = PluginEvent::BeforePluginChanges;
    e.m_pPluginObject = NULL;
    e.m_szPluginFile = NULL;
    s_PluginEvents.Broadcast(e);
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
    e.m_pPluginObject = NULL;
    e.m_szPluginFile = NULL;
    s_PluginEvents.Broadcast(e);
  }
}

ezResult ezPlugin::UnloadPluginInternal(const char* szPluginFile)
{
  BeginPluginChanges();

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
    g_LoadedPlugins[szPluginFile].m_pPluginObject->Uninitialize();
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
    GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin);

    ezOSFile::DeleteFile(sNewPlugin.GetData());
  }

  // if the refcount is zero (ie. we are not 'reloading' plugins), remove the info about the plugin
  if (g_LoadedPlugins[szPluginFile].m_iReferenceCount == 0)
    g_LoadedPlugins.Erase(szPluginFile);

  // Broadcast event: After unloading plugin
  {
    PluginEvent e;
    e.m_EventType = PluginEvent::AfterUnloading;
    e.m_pPluginObject = NULL;
    e.m_szPluginFile = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  ezLog::Success("Plugin '%s' is unloaded.", szPluginFile);

  EndPluginChanges();

  return EZ_SUCCESS;
}

ezResult ezPlugin::LoadPluginInternal(const char* szPluginFile, bool bLoadCopy)
{
  ezStringBuilder sOldPlugin, sNewPlugin;
  GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin);

  if (bLoadCopy)
  {
    // create a copy of the original plugin file
    if (ezOSFile::CopyFile(sOldPlugin.GetData(), sNewPlugin.GetData()) == EZ_FAILURE)
    {
      g_LoadedPlugins.Erase(sNewPlugin.GetData());
      return EZ_FAILURE;
    }
  }

  g_LoadedPlugins[szPluginFile].m_iReferenceCount++;
  BeginPluginChanges();

  // Broadcast Event: Before loading plugin
  {
    PluginEvent e;
    e.m_EventType = PluginEvent::BeforeLoading;
    e.m_pPluginObject = NULL;
    e.m_szPluginFile = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  if (LoadPluginModule(sNewPlugin.GetData(), g_LoadedPlugins[szPluginFile].m_hModule, szPluginFile) == EZ_FAILURE)
  {
    g_LoadedPlugins.Erase(szPluginFile);
    EndPluginChanges();
    
    return EZ_FAILURE;
  }

  // if the platform supports it, store the modification time of the plugin, so that we don't need to reload it, if not necessary
  #if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  {
    ezFileStats stat;
    if (ezOSFile::GetFileStats(sOldPlugin.GetData(), stat) == EZ_SUCCESS)
    {
      g_LoadedPlugins[szPluginFile].m_uiLastModificationTime = stat.m_uiLastModificationTime;
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

        pPlugin->Initialize();

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

    EZ_ASSERT_API(iNewPlugins == 1, "A plugin must contain exactly one instance of an ezPlugin. While loading plugin '%s' %i ezPlugin instances were found.", szPluginFile, iNewPlugins);
  }

  ezLog::Success("Plugin '%s' is loaded.", szPluginFile);
  EndPluginChanges();
  return EZ_SUCCESS;
}

ezResult ezPlugin::LoadPlugin(const char* szPluginFile)
{
  ezStringBuilder sPlugin = szPluginFile;

  EZ_LOG_BLOCK("Loading Plugin");

  if (g_LoadedPlugins.Find(szPluginFile).IsValid())
  {
    g_LoadedPlugins[szPluginFile].m_iReferenceCount++;
    ezLog::Dev("Plugin '%s' already loaded.", szPluginFile);
    return EZ_SUCCESS;
  }

  ezLog::Info("Plugin to load: \"%s\"", szPluginFile);

  return LoadPluginInternal(szPluginFile, true);
}

ezResult ezPlugin::UnloadPlugin(const char* szPluginFile)
{
  EZ_LOG_BLOCK("Unloading Plugin");

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

  ezLog::Info("Plugin to unload: \"%s\"", szPluginFile);
  UnloadPluginInternal(szPluginFile);

  return EZ_SUCCESS;
}

ezResult ezPlugin::ReloadPlugins()
{
  EZ_LOG_BLOCK("Reload Plugins");

  ezLog::Dev("Reloading Plugins");

  ezHybridArray<ezString, 16> PluginsToReload;
  ezResult res = EZ_SUCCESS;

  BeginPluginChanges();

  // find all plugins to reload and unload them
  {
    ezPlugin* pPlugin = ezPlugin::GetFirstInstance();

    while(pPlugin)
    {
      if (pPlugin->IsReloadable())
      {
        bool bModified = true;

        ezStringBuilder sOldPlugin, sNewPlugin;
        GetPluginPaths(pPlugin->m_sLoadedFromFile.GetData(), sOldPlugin, sNewPlugin);

        if (!ezOSFile::Exists(sOldPlugin.GetData()))
        {
          bModified = false;
          res = EZ_FAILURE;
          ezLog::Warning("Plugin '%s' does not exist at the moment. Plugin will not be reloaded.", pPlugin->m_sLoadedFromFile.GetData());
        }
        else
        {
        #if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
 
          ezFileStats stat;
          if (ezOSFile::GetFileStats(sOldPlugin.GetData(), stat) == EZ_SUCCESS)
          {
            if (g_LoadedPlugins[pPlugin->m_sLoadedFromFile.GetData()].m_uiLastModificationTime == stat.m_uiLastModificationTime)
            {
              ezLog::Dev("Plugin '%s' is not modified.", pPlugin->GetPluginFileName());
              bModified = false;
            }
            else
              ezLog::Info("Plugin '%s' is modified, reloading.", pPlugin->GetPluginFileName());
          }
        #endif
        }

        if (bModified)
        {
          ezStringBuilder sBackup = sNewPlugin;
          sBackup.Append(".backup");

          EZ_VERIFY(ezOSFile::CopyFile(sNewPlugin.GetData(), sBackup.GetData()) == EZ_SUCCESS, "Could not create backup of plugin '%s'", pPlugin->GetPluginFileName());

          PluginsToReload.PushBack(pPlugin->m_sLoadedFromFile);
        }
      }

      pPlugin = pPlugin->GetNextInstance();
    }
  }

    // now unload all unloaded plugins
  {
    EZ_LOG_BLOCK("Unloading Plugins");

    for (ezUInt32 i = 0; i < PluginsToReload.GetCount(); ++i)
    {
      EZ_VERIFY (UnloadPluginInternal(PluginsToReload[i].GetData()) == EZ_SUCCESS, "Could not unload plugin '%s'.", PluginsToReload[i].GetData());
    }
  }

  // now load all unloaded plugins
  {
    EZ_LOG_BLOCK("Loading Plugins");

    for (ezUInt32 i = 0; i < PluginsToReload.GetCount(); ++i)
    {
      ezStringBuilder sOldPlugin, sNewPlugin;
      GetPluginPaths(PluginsToReload[i].GetData(), sOldPlugin, sNewPlugin);

      ezStringBuilder sBackup = sNewPlugin;
      sBackup.Append(".backup");

      if (LoadPluginInternal(PluginsToReload[i].GetData(), true) == EZ_FAILURE)
      {
        ezLog::Error("Loading of Plugin '%s' failed. Falling back to backup of previous version.", PluginsToReload[i].GetData());

        res = EZ_FAILURE;

        if (ezOSFile::CopyFile(sBackup.GetData(), sNewPlugin.GetData()) == EZ_SUCCESS)
        {
          EZ_VERIFY (LoadPluginInternal(PluginsToReload[i].GetData(), false) == EZ_SUCCESS, "Could not reload backup of plugin '%s'", PluginsToReload[i].GetData());
        }
      }

      // delete the backup file again
      ezOSFile::DeleteFile(sBackup.GetData());
    }
  }

  EndPluginChanges();

  return res;
}