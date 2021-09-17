#include <Foundation/FoundationPCH.h>

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

struct ModuleData
{
  ezPluginModule m_hModule = 0;
  ezUInt8 m_uiFileNumber = 0;
};

static ezMap<ezString, ModuleData> g_LoadedModules;
static ezDynamicArray<ezString> s_PluginLoadOrder;

ezInt32 ezPlugin::s_iPluginChangeRecursionCounter = 0;
ezUInt32 ezPlugin::m_uiMaxParallelInstances = 32;
ezCopyOnBroadcastEvent<const ezPluginEvent&> ezPlugin::s_PluginEvents;

void ezPlugin::SetMaxParallelInstances(ezUInt32 uiMaxParallelInstances)
{
  m_uiMaxParallelInstances = ezMath::Max(1u, uiMaxParallelInstances);
}

void ezPlugin::InitializeStaticallyLinkedPlugins()
{
  ezPlugin::ConfigureNewPlugins("Executable");
}

ezPlugin::ezPlugin()
{
}

void ezPlugin::Initialize()
{
  if (m_bInitialized)
    return;

  m_bInitialized = true;

  for (ezUInt32 i = 0; true; ++i)
  {
    if (GetPluginDependency(i) == nullptr)
      break;

    // TODO: ignore ??
    LoadPlugin(GetPluginDependency(i)).IgnoreResult();
  }

  OnPluginLoaded();
}

void ezPlugin::Uninitialize()
{
  if (!m_bInitialized)
    return;

  OnPluginUnloaded();

  m_bInitialized = false;
}

void ezPlugin::BeginPluginChanges()
{
  if (s_iPluginChangeRecursionCounter == 0)
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::BeforePluginChanges;
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
    s_PluginEvents.Broadcast(e);
  }
}

void ezPlugin::OnPluginLoaded()
{
}

void ezPlugin::OnPluginUnloaded()
{
}

ezResult ezPlugin::UnloadPluginInternal(const char* szPluginFile)
{
  if (!g_LoadedModules.Find(szPluginFile).IsValid())
    return EZ_SUCCESS;

  ezLog::Debug("Plugin to unload: \"{0}\"", szPluginFile);

  BeginPluginChanges();

  // Broadcast event: Before unloading plugin
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::BeforeUnloading;
    e.m_szPluginBinary = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: Startup Shutdown
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::StartupShutdown;
    e.m_szPluginBinary = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: After Startup Shutdown
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::AfterStartupShutdown;
    e.m_szPluginBinary = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // uninitialize all plugin objects that originated from this binary
  for (ezPlugin* pPlugin = ezPlugin::GetFirstInstance(); pPlugin != nullptr; pPlugin = pPlugin->GetNextInstance())
  {
    if (ezStringUtils::IsEqual_NoCase(pPlugin->GetOriginBinary(), szPluginFile))
    {
      pPlugin->Uninitialize();
    }
  }

  // unload the plugin module
  if (UnloadPluginModule(g_LoadedModules[szPluginFile].m_hModule, szPluginFile) == EZ_FAILURE)
  {
    ezLog::Error("Unloading plugin module '{}' failed.", szPluginFile);

    EndPluginChanges();
    return EZ_FAILURE;
  }

  // delete the plugin copy that we had loaded
  {
    ezStringBuilder sOldPlugin, sNewPlugin;
    GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin, g_LoadedModules[szPluginFile].m_uiFileNumber);

    ezOSFile::DeleteFile(sNewPlugin.GetData()).IgnoreResult();
  }

  // Broadcast event: After unloading plugin
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::AfterUnloading;
    e.m_szPluginBinary = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  for (ezPlugin* pPlugin = ezPlugin::GetFirstInstance(); pPlugin != nullptr; pPlugin = pPlugin->GetNextInstance())
  {
    if (ezStringUtils::IsEqual_NoCase(pPlugin->GetOriginBinary(), szPluginFile))
    {
      EZ_REPORT_FAILURE("ezPlugin object loaded from '{}' still exists after module was unloaded.", szPluginFile);
    }
  }

  ezLog::Success("Plugin '{0}' is unloaded.", szPluginFile);
  g_LoadedModules.Remove(szPluginFile);

  EndPluginChanges();

  return EZ_SUCCESS;
}

ezResult ezPlugin::LoadPluginInternal(const char* szPluginFile, ezBitflags<ezPluginLoadFlags> flags)
{
  ezUInt8 uiFileNumber = 0;

  ezStringBuilder sOldPlugin, sNewPlugin;
  GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin, uiFileNumber);

  if (!ezOSFile::ExistsFile(sOldPlugin))
  {
    ezLog::Error("The plugin '{0}' does not exist.", szPluginFile);
    return EZ_FAILURE;
  }

  if (flags.IsSet(ezPluginLoadFlags::LoadCopy))
  {
    // create a copy of the original plugin file
    const ezUInt8 uiMaxParallelInstances = static_cast<ezUInt8>(ezPlugin::m_uiMaxParallelInstances);
    for (uiFileNumber = 0; uiFileNumber < uiMaxParallelInstances; ++uiFileNumber)
    {
      GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin, uiFileNumber);
      if (ezOSFile::CopyFile(sOldPlugin.GetData(), sNewPlugin.GetData()) == EZ_SUCCESS)
        goto success;
    }

    ezLog::Error("Could not copy the plugin file '{0}' to '{1}' (and all previous file numbers). Plugin MaxParallelInstances is set to {2}.", sOldPlugin, sNewPlugin, ezPlugin::m_uiMaxParallelInstances);

    g_LoadedModules.Remove(sNewPlugin);
    return EZ_FAILURE;
  }
  else
  {
    sNewPlugin = sOldPlugin;
  }

success:

  g_LoadedModules[szPluginFile].m_uiFileNumber = uiFileNumber;

  BeginPluginChanges();

  // Broadcast Event: Before loading plugin
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::BeforeLoading;
    e.m_szPluginBinary = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  if (LoadPluginModule(sNewPlugin.GetData(), g_LoadedModules[szPluginFile].m_hModule, szPluginFile) == EZ_FAILURE)
  {
    // loaded, but failed
    g_LoadedModules[szPluginFile].m_hModule = 0;
    EndPluginChanges();

    return EZ_FAILURE;
  }

  // Find all known plugin objects
  ConfigureNewPlugins(szPluginFile);

  ezLog::Success("Plugin '{0}' is loaded.", szPluginFile);
  EndPluginChanges();
  return EZ_SUCCESS;
}

void ezPlugin::ConfigureNewPlugins(const char* szOriginBinary)
{
  ezHybridArray<ezPlugin*, 4> newPluginsNow;

  for (ezPlugin* pPlugin = ezPlugin::GetFirstInstance(); pPlugin != nullptr; pPlugin = pPlugin->GetNextInstance())
  {
    if (!pPlugin->m_bInitialized)
    {
      pPlugin->m_sOriginBinary = szOriginBinary;

      newPluginsNow.PushBack(pPlugin);
    }
  }

  if (newPluginsNow.IsEmpty())
  {
    // make sure the events are sent, even if there is no plugin definition in the DLL
    newPluginsNow.PushBack(nullptr);
  }

  for (ezPlugin* pPlugin : newPluginsNow)
  {
    // Broadcast Event: After loading plugin, before init
    {
      ezPluginEvent e;
      e.m_EventType = ezPluginEvent::AfterLoadingBeforeInit;
      e.m_szPluginBinary = szOriginBinary;
      s_PluginEvents.Broadcast(e);
    }

    // this may trigger recursive plugin loading
    if (pPlugin)
    {
      EZ_ASSERT_DEV(!pPlugin->m_bInitialized, "Something went wrong");
      pPlugin->Initialize();
    }

    // Broadcast Event: After loading plugin
    {
      ezPluginEvent e;
      e.m_EventType = ezPluginEvent::AfterLoading;
      e.m_szPluginBinary = szOriginBinary;
      s_PluginEvents.Broadcast(e);
    }
  }
}

bool ezPlugin::ExistsPluginFile(const char* szPluginFile)
{
  ezStringBuilder sOldPlugin, sNewPlugin;
  GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin, 0);

  return ezOSFile::ExistsFile(sOldPlugin);
}

ezResult ezPlugin::LoadPlugin(const char* szPluginFile, ezBitflags<ezPluginLoadFlags> flags /*= ezPluginLoadFlags::Default*/)
{
  if (flags.IsSet(ezPluginLoadFlags::PluginIsOptional))
  {
    // early out without logging an error

    if (!ExistsPluginFile(szPluginFile))
      return EZ_FAILURE;
  }

  EZ_LOG_BLOCK("Loading Plugin", szPluginFile);

  if (g_LoadedModules.Find(szPluginFile).IsValid())
  {
    ezLog::Debug("Plugin '{0}' already loaded.", szPluginFile);
    return EZ_SUCCESS;
  }

  // make sure this is done first
  InitializeStaticallyLinkedPlugins();

  ezLog::Debug("Plugin to load: \"{0}\"", szPluginFile);

  // make sure to use a static string pointer from now on, that stays where it is
  szPluginFile = g_LoadedModules.FindOrAdd(szPluginFile).Key();

  ezResult res = LoadPluginInternal(szPluginFile, flags);

  if (res.Succeeded())
  {
    s_PluginLoadOrder.PushBack(szPluginFile);
  }

  return res;
}

void ezPlugin::UnloadAllPlugins()
{
  for (ezUInt32 i = s_PluginLoadOrder.GetCount(); i > 0; --i)
  {
    if (ezPlugin::UnloadPluginInternal(s_PluginLoadOrder[i - 1]).Failed())
    {
      // not sure what to do
    }
  }

  s_PluginLoadOrder.Clear();
  g_LoadedModules.Clear();

  // also shut down all plugin objects that are statically linked
  for (ezPlugin* pPlugin = ezPlugin::GetFirstInstance(); pPlugin != nullptr; pPlugin = pPlugin->GetNextInstance())
  {
    pPlugin->Uninitialize();
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Plugin);
