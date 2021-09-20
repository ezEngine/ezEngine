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

struct ModuleData
{
  ezPluginModule m_hModule = 0;
  ezUInt8 m_uiFileNumber = 0;
  bool m_bCalledOnLoad = false;
  ezHybridArray<ezPluginInitCallback, 2> m_OnLoadCB;
  ezHybridArray<ezPluginInitCallback, 2> m_OnUnloadCB;
  ezHybridArray<ezString, 2> m_sPluginDependencies;

  void Initialize();
  void Uninitialize();
};

static ModuleData g_StaticModule;
static ModuleData* g_pCurrentlyLoadingModule = nullptr;
static ezMap<ezString, ModuleData> g_LoadedModules;
static ezDynamicArray<ezString> s_PluginLoadOrder;
static ezUInt32 s_uiMaxParallelInstances = 32;
static ezInt32 s_iPluginChangeRecursionCounter = 0;

ezCopyOnBroadcastEvent<const ezPluginEvent&> ezPlugin::s_PluginEvents;

void ezPlugin::SetMaxParallelInstances(ezUInt32 uiMaxParallelInstances)
{
  s_uiMaxParallelInstances = ezMath::Max(1u, uiMaxParallelInstances);
}

void ezPlugin::InitializeStaticallyLinkedPlugins()
{
  g_StaticModule.Initialize();
}

void ModuleData::Initialize()
{
  if (m_bCalledOnLoad)
    return;

  m_bCalledOnLoad = true;

  for (const auto& dep : m_sPluginDependencies)
  {
    // TODO: ignore ??
    ezPlugin::LoadPlugin(dep).IgnoreResult();
  }

  for (auto cb : m_OnLoadCB)
  {
    cb();
  }
}

void ModuleData::Uninitialize()
{
  if (!m_bCalledOnLoad)
    return;

  for (auto cb : m_OnUnloadCB)
  {
    cb();
  }

  m_bCalledOnLoad = false;
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

ezResult ezPlugin::UnloadPluginInternal(const char* szPluginFile)
{
  auto thisMod = g_LoadedModules.Find(szPluginFile);

  if (!thisMod.IsValid())
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

  thisMod.Value().Uninitialize();

  // unload the plugin module
  if (UnloadPluginModule(thisMod.Value().m_hModule, szPluginFile) == EZ_FAILURE)
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

  ezLog::Success("Plugin '{0}' is unloaded.", szPluginFile);
  g_LoadedModules.Remove(thisMod);

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
    const ezUInt8 uiMaxParallelInstances = static_cast<ezUInt8>(s_uiMaxParallelInstances);
    for (uiFileNumber = 0; uiFileNumber < uiMaxParallelInstances; ++uiFileNumber)
    {
      GetPluginPaths(szPluginFile, sOldPlugin, sNewPlugin, uiFileNumber);
      if (ezOSFile::CopyFile(sOldPlugin.GetData(), sNewPlugin.GetData()) == EZ_SUCCESS)
        goto success;
    }

    ezLog::Error("Could not copy the plugin file '{0}' to '{1}' (and all previous file numbers). Plugin MaxParallelInstances is set to {2}.", sOldPlugin, sNewPlugin, s_uiMaxParallelInstances);

    g_LoadedModules.Remove(sNewPlugin);
    return EZ_FAILURE;
  }
  else
  {
    sNewPlugin = sOldPlugin;
  }

success:

  auto& thisMod = g_LoadedModules[szPluginFile];
  thisMod.m_uiFileNumber = uiFileNumber;

  BeginPluginChanges();

  // Broadcast Event: Before loading plugin
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::BeforeLoading;
    e.m_szPluginBinary = szPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  g_pCurrentlyLoadingModule = &thisMod;

  if (LoadPluginModule(sNewPlugin.GetData(), g_pCurrentlyLoadingModule->m_hModule, szPluginFile) == EZ_FAILURE)
  {
    // loaded, but failed
    g_pCurrentlyLoadingModule = nullptr;
    thisMod.m_hModule = 0;
    EndPluginChanges();

    return EZ_FAILURE;
  }

  g_pCurrentlyLoadingModule = nullptr;

  {
    // Broadcast Event: After loading plugin, before init
    {
      ezPluginEvent e;
      e.m_EventType = ezPluginEvent::AfterLoadingBeforeInit;
      e.m_szPluginBinary = szPluginFile;
      s_PluginEvents.Broadcast(e);
    }

    thisMod.Initialize();

    // Broadcast Event: After loading plugin
    {
      ezPluginEvent e;
      e.m_EventType = ezPluginEvent::AfterLoading;
      e.m_szPluginBinary = szPluginFile;
      s_PluginEvents.Broadcast(e);
    }
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

  EZ_ASSERT_DEBUG(g_LoadedModules.IsEmpty(), "Not all plugins were unloaded somehow.");

  for (auto mod : g_LoadedModules)
  {
    mod.Value().Uninitialize();
  }

  // also shut down all plugin objects that are statically linked
  g_StaticModule.Uninitialize();

  s_PluginLoadOrder.Clear();
  g_LoadedModules.Clear();
}

ezPlugin::Init::Init(ezPluginInitCallback OnLoadOrUnloadCB, bool bOnLoad)
{
  ModuleData* pMD = g_pCurrentlyLoadingModule ? g_pCurrentlyLoadingModule : &g_StaticModule;

  if (bOnLoad)
    pMD->m_OnLoadCB.PushBack(OnLoadOrUnloadCB);
  else
    pMD->m_OnUnloadCB.PushBack(OnLoadOrUnloadCB);
}

ezPlugin::Init::Init(const char* szAddPluginDependency)
{
  ModuleData* pMD = g_pCurrentlyLoadingModule ? g_pCurrentlyLoadingModule : &g_StaticModule;

  pMD->m_sPluginDependencies.PushBack(szAddPluginDependency);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Plugin);
