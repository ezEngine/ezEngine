#include <PCH.h>

#include <GameEngine/GameApplication/GameApplicationBase.h>

#include <Core/Application/Config/FileSystemConfig.h>
#include <Core/Application/Config/PluginConfig.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Types/TagRegistry.h>
#include <GameEngine/Configuration/InputConfig.h>

ezString ezGameApplicationBase::GetBaseDataDirectoryPath() const
{
  return ">sdk/Data/Base";
}

void ezGameApplicationBase::ExecuteInitFunctions()
{
  Init_PlatformProfile_SetPreferred();
  Init_ConfigureLogging();
  Init_ConfigureTelemetry();
  Init_FileSystem_SetSpecialDirs();
  Init_FileSystem_SetDataDirFactories();
  Init_LoadRequiredPlugins();
  Init_ConfigureAssetManagement();
  Init_FileSystem_ConfigureDataDirs();
  Init_LoadProjectPlugins();
  Init_PlatformProfile_LoadForRuntime();
  Init_ConfigureInput();
  Init_ConfigureTags();
  Init_ConfigureCVars();
  Init_SetupGraphicsDevice();
  Init_SetupDefaultResources();
}

void ezGameApplicationBase::Init_PlatformProfile_SetPreferred()
{
  m_PlatformProfile.m_sName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-profile", 0, "PC");
  m_PlatformProfile.AddMissingConfigs();
}

void ezGameApplicationBase::Init_ConfigureLogging()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
#endif
}

void ezGameApplicationBase::Init_ConfigureTelemetry()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezTelemetry::CreateServer();
#endif
}

void ezGameApplicationBase::Init_FileSystem_SetSpecialDirs()
{
  ezFileSystem::SetSpecialDirectory("project", FindProjectDirectory());
}

void ezGameApplicationBase::Init_FileSystem_SetDataDirFactories()
{
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
}

void ezGameApplicationBase::Init_ConfigureAssetManagement() {}

void ezGameApplicationBase::Init_LoadRequiredPlugins() {}

void ezGameApplicationBase::Init_FileSystem_ConfigureDataDirs()
{
  // ">appdir/" and ">user/" are built-in special directories
  // see ezFileSystem::ResolveSpecialDirectory

  const ezStringBuilder sUserDataPath(">user/", GetApplicationName());

  ezFileSystem::CreateDirectoryStructure(sUserDataPath);

  ezString writableBinRoot = ">appdir/";
  ezString shaderCacheRoot = ">appdir/";

#if EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // On platforms where this is disabled, one can usually only write to the user directory
  // e.g. on UWP and mobile platforms
  writableBinRoot = sUserDataPath;
  shaderCacheRoot = sUserDataPath;
#endif

  // for absolute paths, read-only
  ezFileSystem::AddDataDirectory("", "GameApplicationBase", ":", ezFileSystem::ReadOnly);

  // ":bin/" : writing to the binary directory
  ezFileSystem::AddDataDirectory(writableBinRoot, "GameApplicationBase", "bin", ezFileSystem::AllowWrites);

  // ":shadercache/" for reading and writing shader files
  ezFileSystem::AddDataDirectory(shaderCacheRoot, "GameApplicationBase", "shadercache", ezFileSystem::AllowWrites);

  // ":appdata/" for reading and writing app user data
  ezFileSystem::AddDataDirectory(sUserDataPath, "GameApplicationBase", "appdata", ezFileSystem::AllowWrites);

  // ":base/" for reading the core engine files
  ezFileSystem::AddDataDirectory(GetBaseDataDirectoryPath(), "GameApplicationBase", "base", ezFileSystem::DataDirUsage::ReadOnly);

  // ":project/" for reading the project specific files
  ezFileSystem::AddDataDirectory(">project/", "GameApplicationBase", "project", ezFileSystem::DataDirUsage::ReadOnly);

  {
    ezApplicationFileSystemConfig appFileSystemConfig;
    appFileSystemConfig.Load();

    // get rid of duplicates that we already hard-coded above
    for (ezUInt32 i = appFileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
    {
      const ezString name = appFileSystemConfig.m_DataDirs[i - 1].m_sRootName;
      if (name.IsEqual_NoCase(":") || name.IsEqual_NoCase("bin") || name.IsEqual_NoCase("shadercache") || name.IsEqual_NoCase("appdata") ||
          name.IsEqual_NoCase("base") || name.IsEqual_NoCase("project"))
      {
        appFileSystemConfig.m_DataDirs.RemoveAtAndCopy(i - 1);
      }
    }

    appFileSystemConfig.Apply();
  }
}

void ezGameApplicationBase::Init_LoadProjectPlugins()
{
  ezApplicationPluginConfig appPluginConfig;
  appPluginConfig.Load();
  appPluginConfig.SetOnlyLoadManualPlugins(true);
  appPluginConfig.Apply();
}

void ezGameApplicationBase::Init_PlatformProfile_LoadForRuntime()
{
  const ezStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.m_sName, ".ezProfile");
  m_PlatformProfile.AddMissingConfigs();
  m_PlatformProfile.LoadForRuntime(sRuntimeProfileFile);
}

namespace
{
  const char* g_szInputSet = "GameApp";
  const char* g_szCloseAppAction = "CloseApp";
  const char* g_szShowConsole = "ShowConsole";
  const char* g_szShowFpsAction = "ShowFps";
  const char* g_szReloadResourcesAction = "ReloadResources";
  const char* g_szCaptureProfilingAction = "CaptureProfiling";
  const char* g_szTakeScreenshot = "TakeScreenshot";
} // namespace

void ezGameApplicationBase::Init_ConfigureInput()
{
  ezInputActionConfig config;

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
  ezInputManager::SetInputActionConfig(g_szInputSet, g_szCloseAppAction, config, true);

  // the tilde has problematic behavior on keyboards where it is a hat (^)
  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF1;
  ezInputManager::SetInputActionConfig("Console", g_szShowConsole, config, true);

  // in the editor we cannot use F5, because that is already 'run application'
  // so we use F4 there, and it should be consistent here
  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF4;
  ezInputManager::SetInputActionConfig(g_szInputSet, g_szReloadResourcesAction, config, true);

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF5;
  ezInputManager::SetInputActionConfig(g_szInputSet, g_szShowFpsAction, config, true);

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF8;
  ezInputManager::SetInputActionConfig(g_szInputSet, g_szCaptureProfilingAction, config, true);

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF12;
  ezInputManager::SetInputActionConfig(g_szInputSet, g_szTakeScreenshot, config, true);

  {
    ezFileReader file;
    if (file.Open(":project/InputConfig.ddl").Succeeded())
    {
      ezHybridArray<ezGameAppInputConfig, 32> InputActions;

      ezGameAppInputConfig::ReadFromDDL(file, InputActions);
      ezGameAppInputConfig::ApplyAll(InputActions);
    }
  }
}

void ezGameApplicationBase::Init_ConfigureTags()
{
  EZ_LOG_BLOCK("Reading Tags", "Tags.ddl");

  ezFileReader file;
  if (file.Open(":project/Tags.ddl").Failed())
  {
    ezLog::Dev("'Tags.ddl' does not exist");
    return;
  }

  ezStringBuilder tmp;

  ezOpenDdlReader reader;
  if (reader.ParseDocument(file).Failed())
  {
    ezLog::Error("Failed to parse DDL data in tags file");
    return;
  }

  const ezOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const ezOpenDdlReaderElement* pTags = pRoot->GetFirstChild(); pTags != nullptr; pTags = pTags->GetSibling())
  {
    if (!pTags->IsCustomType("Tag"))
      continue;

    const ezOpenDdlReaderElement* pName = pTags->FindChildOfType(ezOpenDdlPrimitiveType::String, "Name");

    if (!pName)
    {
      ezLog::Error("Incomplete tag declaration!");
      continue;
    }

    tmp = pName->GetPrimitivesString()[0];
    ezTagRegistry::GetGlobalRegistry().RegisterTag(tmp);
  }
}

void ezGameApplicationBase::Init_ConfigureCVars()
{
  ezCVar::SetStorageFolder(":appdata/CVars");
  ezCVar::LoadCVars();
}

void ezGameApplicationBase::Init_SetupDefaultResources() {}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezGameApplicationBase::Deinit_UnloadPlugins()
{
  ezSet<ezString> ToUnload;

  // if a plugin is linked statically (which happens mostly in an editor context)
  // then it cannot be unloaded and the ezPlugin instance won't ever go away
  // however, ezPlugin::UnloadPlugin will always return that it is already unloaded, so we can just skip it there
  // all other plugins must be unloaded as often as their refcount, though
  ezStringBuilder s;
  ezPlugin* pPlugin = ezPlugin::GetFirstInstance();
  while (pPlugin != nullptr)
  {
    s = ezPlugin::GetFirstInstance()->GetPluginName();
    ToUnload.Insert(s);

    pPlugin = pPlugin->GetNextInstance();
  }

  ezString temp;
  while (!ToUnload.IsEmpty())
  {
    auto it = ToUnload.GetIterator();

    ezInt32 iRefCount = 0;
    EZ_VERIFY(ezPlugin::UnloadPlugin(it.Key(), &iRefCount).Succeeded(), "Failed to unload plugin '{0}'", s);

    if (iRefCount == 0)
      ToUnload.Remove(it);
  }
}

void ezGameApplicationBase::Deinit_ShutdownLogging() {}
