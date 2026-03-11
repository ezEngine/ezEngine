#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

#include <Plugin_Platform.inl>

ezResult UnloadPluginModule(ezPluginModule& ref_pModule, ezStringView sPluginFile);
ezResult LoadPluginModule(ezStringView sFileToLoad, ezPluginModule& ref_pModule, ezStringView sPluginFile);

ezDynamicArray<ezString>& GetStaticPlugins()
{
  static ezDynamicArray<ezString> s_StaticPlugins;
  return s_StaticPlugins;
}

struct ModuleData
{
  ezPluginModule m_hModule = 0;
  ezUInt8 m_uiFileNumber = 0;
  bool m_bCalledOnLoad = false;
  ezHybridArray<ezPluginInitCallback, 2> m_OnLoadCB;
  ezHybridArray<ezPluginInitCallback, 2> m_OnUnloadCB;
  ezHybridArray<ezString, 2> m_sPluginDependencies;
  ezBitflags<ezPluginLoadFlags> m_LoadFlags;

  void Initialize();
  void Uninitialize();
};

static ModuleData g_StaticModule;
static ModuleData* g_pCurrentlyLoadingModule = nullptr;
static ezMap<ezString, ModuleData> g_LoadedModules;
static ezDynamicArray<ezString> s_PluginLoadOrder;
static ezUInt32 s_uiMaxParallelInstances = 32;
static ezInt32 s_iPluginChangeRecursionCounter = 0;

ezCopyOnBroadcastEvent<const ezPluginEvent&> s_PluginEvents;

void ezPlugin::SetMaxParallelInstances(ezUInt32 uiMaxParallelInstances)
{
  s_uiMaxParallelInstances = ezMath::Max(1u, uiMaxParallelInstances);
}

void ezPlugin::InitializeStaticallyLinkedPlugins()
{
  if (!g_StaticModule.m_bCalledOnLoad)
  {
    // We need to trigger the ezPlugin events to make sure the sub-systems are initialized at least once.
    ezPlugin::BeginPluginChanges();
    EZ_SCOPE_EXIT(ezPlugin::EndPluginChanges());
    g_StaticModule.Initialize();

#if EZ_DISABLED(EZ_COMPILE_ENGINE_AS_DLL)
    EZ_LOG_BLOCK("Initialize Statically Linked Plugins");
    // Merely add dummy entries so plugins can be enumerated etc.
    for (ezStringView sPlugin : GetStaticPlugins())
    {
      g_LoadedModules.FindOrAdd(sPlugin);
      ezLog::Debug("Plugin '{0}' statically linked.", sPlugin);
    }
#endif
  }
}

void ezPlugin::GetAllPluginInfos(ezDynamicArray<PluginInfo>& ref_infos)
{
  ref_infos.Clear();

  ref_infos.Reserve(g_LoadedModules.GetCount());

  for (auto mod : g_LoadedModules)
  {
    auto& pi = ref_infos.ExpandAndGetRef();
    pi.m_sName = mod.Key();
    pi.m_sDependencies = mod.Value().m_sPluginDependencies;
    pi.m_LoadFlags = mod.Value().m_LoadFlags;
  }
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

  for (ezUInt32 i = m_OnUnloadCB.GetCount(); i > 0; --i)
  {
    m_OnUnloadCB[i - 1]();
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

static ezResult UnloadPluginInternal(ezStringView sPluginFile)
{
  auto thisMod = g_LoadedModules.Find(sPluginFile);

  if (!thisMod.IsValid())
    return EZ_SUCCESS;

  ezLog::Debug("Plugin to unload: \"{0}\"", sPluginFile);

  ezPlugin::BeginPluginChanges();
  EZ_SCOPE_EXIT(ezPlugin::EndPluginChanges());

  // Broadcast event: Before unloading plugin
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::BeforeUnloading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: Startup Shutdown
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::StartupShutdown;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: After Startup Shutdown
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::AfterStartupShutdown;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  thisMod.Value().Uninitialize();

  // unload the plugin module
  if (UnloadPluginModule(thisMod.Value().m_hModule, sPluginFile) == EZ_FAILURE)
  {
    ezLog::Error("Unloading plugin module '{}' failed.", sPluginFile);
    return EZ_FAILURE;
  }

  // delete the plugin copy that we had loaded
  if (ezPlugin::PlatformNeedsPluginCopy())
  {
    ezStringBuilder sOriginalFile, sCopiedFile;
    ezPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, g_LoadedModules[sPluginFile].m_uiFileNumber);

    ezOSFile::DeleteFile(sCopiedFile).IgnoreResult();
  }

  // Broadcast event: After unloading plugin
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::AfterUnloading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  ezLog::Success("Plugin '{0}' is unloaded.", sPluginFile);
  g_LoadedModules.Remove(thisMod);

  return EZ_SUCCESS;
}

#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)

static ezResult LoadPluginInternal(ezStringView sPluginFile, ezBitflags<ezPluginLoadFlags> flags)
{
  ezUInt8 uiFileNumber = 0;

  ezStringBuilder sOriginalFile, sCopiedFile;
  ezPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, uiFileNumber);

  if (!ezOSFile::ExistsFile(sOriginalFile))
  {
    ezLog::Error("The plugin '{0}' does not exist.", sPluginFile);
    return EZ_FAILURE;
  }

  if (ezPlugin::PlatformNeedsPluginCopy() && flags.IsSet(ezPluginLoadFlags::LoadCopy))
  {
    // create a copy of the original plugin file
    const ezUInt8 uiMaxParallelInstances = static_cast<ezUInt8>(s_uiMaxParallelInstances);
    for (uiFileNumber = 0; uiFileNumber < uiMaxParallelInstances; ++uiFileNumber)
    {
      ezPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, uiFileNumber);
      if (ezOSFile::CopyFile(sOriginalFile, sCopiedFile) == EZ_SUCCESS)
        goto success;
    }

    ezLog::Error("Could not copy the plugin file '{0}' to '{1}' (and all previous file numbers). Plugin MaxParallelInstances is set to {2}.", sOriginalFile, sCopiedFile, s_uiMaxParallelInstances);

    g_LoadedModules.Remove(sCopiedFile);
    return EZ_FAILURE;
  }
  else
  {
    sCopiedFile = sOriginalFile;
  }

success:

  auto& thisMod = g_LoadedModules[sPluginFile];
  thisMod.m_uiFileNumber = uiFileNumber;
  thisMod.m_LoadFlags = flags;

  ezPlugin::BeginPluginChanges();
  EZ_SCOPE_EXIT(ezPlugin::EndPluginChanges());

  // Broadcast Event: Before loading plugin
  {
    ezPluginEvent e;
    e.m_EventType = ezPluginEvent::BeforeLoading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  g_pCurrentlyLoadingModule = &thisMod;

  if (LoadPluginModule(sCopiedFile, g_pCurrentlyLoadingModule->m_hModule, sPluginFile) == EZ_FAILURE)
  {
    // loaded, but failed
    g_pCurrentlyLoadingModule = nullptr;
    thisMod.m_hModule = 0;

    return EZ_FAILURE;
  }

  g_pCurrentlyLoadingModule = nullptr;

  {
    // Broadcast Event: After loading plugin, before init
    {
      ezPluginEvent e;
      e.m_EventType = ezPluginEvent::AfterLoadingBeforeInit;
      e.m_sPluginBinary = sPluginFile;
      s_PluginEvents.Broadcast(e);
    }

    thisMod.Initialize();

    // Broadcast Event: After loading plugin
    {
      ezPluginEvent e;
      e.m_EventType = ezPluginEvent::AfterLoading;
      e.m_sPluginBinary = sPluginFile;
      s_PluginEvents.Broadcast(e);
    }
  }

  ezLog::Success("Plugin '{0}' is loaded.", sPluginFile);
  return EZ_SUCCESS;
}

#endif

bool ezPlugin::ExistsPluginFile(ezStringView sPluginFile)
{
  ezStringBuilder sOriginalFile, sCopiedFile;
  GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, 0);

  return ezOSFile::ExistsFile(sOriginalFile);
}

ezResult ezPlugin::LoadPlugin(ezStringView sPluginFile, ezBitflags<ezPluginLoadFlags> flags /*= ezPluginLoadFlags::Default*/)
{
  EZ_LOG_BLOCK("Loading Plugin", sPluginFile);

  // make sure this is done first
  InitializeStaticallyLinkedPlugins();

  if (g_LoadedModules.Find(sPluginFile).IsValid())
  {
    ezLog::Debug("Plugin '{0}' already loaded.", sPluginFile);
    return EZ_SUCCESS;
  }

#if EZ_DISABLED(EZ_COMPILE_ENGINE_AS_DLL)
  // #TODO EZ_COMPILE_ENGINE_AS_DLL and being able to load plugins are not necessarily the same thing.
  EZ_IGNORE_UNUSED(flags);
  return EZ_FAILURE;
#else

  if (flags.IsSet(ezPluginLoadFlags::PluginIsOptional))
  {
    // early out without logging an error
    if (!ExistsPluginFile(sPluginFile))
      return EZ_FAILURE;
  }

  ezLog::Debug("Plugin to load: \"{0}\"", sPluginFile);

  // make sure to use a static string pointer from now on, that stays where it is
  sPluginFile = g_LoadedModules.FindOrAdd(sPluginFile).Key();

  ezResult res = LoadPluginInternal(sPluginFile, flags);

  if (res.Succeeded())
  {
    s_PluginLoadOrder.PushBack(sPluginFile);
  }
  else
  {
    // If we failed to load the plugin, it shouldn't be in the loaded modules list
    g_LoadedModules.Remove(sPluginFile);
  }

  return res;
#endif
}

void ezPlugin::UnloadAllPlugins()
{
  BeginPluginChanges();
  EZ_SCOPE_EXIT(EndPluginChanges());

  for (ezUInt32 i = s_PluginLoadOrder.GetCount(); i > 0; --i)
  {
    if (UnloadPluginInternal(s_PluginLoadOrder[i - 1]).Failed())
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

const ezCopyOnBroadcastEvent<const ezPluginEvent&>& ezPlugin::Events()
{
  return s_PluginEvents;
}

ezPlugin::Init::Init(ezPluginInitCallback onLoadOrUnloadCB, bool bOnLoad)
{
  ModuleData* pMD = g_pCurrentlyLoadingModule ? g_pCurrentlyLoadingModule : &g_StaticModule;

  if (bOnLoad)
    pMD->m_OnLoadCB.PushBack(onLoadOrUnloadCB);
  else
    pMD->m_OnUnloadCB.PushBack(onLoadOrUnloadCB);
}

ezPlugin::Init::Init(const char* szAddPluginDependency)
{
  ModuleData* pMD = g_pCurrentlyLoadingModule ? g_pCurrentlyLoadingModule : &g_StaticModule;

  pMD->m_sPluginDependencies.PushBack(szAddPluginDependency);
}

#if EZ_DISABLED(EZ_COMPILE_ENGINE_AS_DLL)
ezPluginRegister::ezPluginRegister(const char* szAddPlugin)
{
  if (g_pCurrentlyLoadingModule == nullptr)
  {
    GetStaticPlugins().PushBack(szAddPlugin);
  }
}
#endif
