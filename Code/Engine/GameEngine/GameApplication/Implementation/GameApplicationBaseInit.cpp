#include <GameEnginePCH.h>

#include <GameEngine/GameApplication/GameApplicationBase.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/WorldModuleConfig.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Types/TagRegistry.h>

ezString ezGameApplicationBase::GetBaseDataDirectoryPath() const
{
  return ">sdk/Data/Base";
}

ezString ezGameApplicationBase::GetProjectDataDirectoryPath() const
{
  return ">project/";
}

void ezGameApplicationBase::ExecuteInitFunctions()
{
  Init_PlatformProfile_SetPreferred();
  Init_ConfigureTelemetry();
  Init_FileSystem_SetSpecialDirs();
  Init_LoadRequiredPlugins();
  Init_ConfigureAssetManagement();
  Init_FileSystem_ConfigureDataDirs();
  Init_LoadWorldModuleConfig();
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

void ezGameApplicationBase::BaseInit_ConfigureLogging()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
#endif
}

void ezGameApplicationBase::Init_ConfigureTelemetry()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezTelemetry::s_uiPort = ezCommandLineUtils::GetGlobalInstance()->GetIntOption("-TelemetryPort", ezTelemetry::s_uiPort);
  ezTelemetry::SetServerName(GetApplicationName());
  ezTelemetry::CreateServer();
#endif
}

void ezGameApplicationBase::Init_FileSystem_SetSpecialDirs()
{
  ezFileSystem::SetSpecialDirectory("project", FindProjectDirectory());
}

void ezGameApplicationBase::Init_ConfigureAssetManagement() {}

void ezGameApplicationBase::Init_LoadRequiredPlugins()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezPlugin::LoadOptionalPlugin("XBoxControllerPlugin");
#endif
}

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
  ezFileSystem::AddDataDirectory(GetProjectDataDirectoryPath(), "GameApplicationBase", "project", ezFileSystem::DataDirUsage::ReadOnly);

  // ":plugins/" for plugin specific data (optional, if it exists)
  {
    ezStringBuilder dir;
    ezFileSystem::ResolveSpecialDirectory(">sdk/Data/Plugins", dir);
    if (ezOSFile::ExistsDirectory(dir))
    {
      ezFileSystem::AddDataDirectory(">sdk/Data/Plugins", "GameApplicationBase", "plugins", ezFileSystem::DataDirUsage::ReadOnly);
    }
  }

  {
    ezApplicationFileSystemConfig appFileSystemConfig;
    appFileSystemConfig.Load();

    // get rid of duplicates that we already hard-coded above
    for (ezUInt32 i = appFileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
    {
      const ezString name = appFileSystemConfig.m_DataDirs[i - 1].m_sRootName;
      if (name.IsEqual_NoCase(":") || name.IsEqual_NoCase("bin") || name.IsEqual_NoCase("shadercache") || name.IsEqual_NoCase("appdata") ||
          name.IsEqual_NoCase("base") || name.IsEqual_NoCase("project") || name.IsEqual_NoCase("plugins"))
      {
        appFileSystemConfig.m_DataDirs.RemoveAtAndCopy(i - 1);
      }
    }

    appFileSystemConfig.Apply();
  }
}

void ezGameApplicationBase::Init_LoadWorldModuleConfig()
{
  ezWorldModuleConfig worldModuleConfig;
  worldModuleConfig.Load();
  worldModuleConfig.Apply();
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

void ezGameApplicationBase::Init_ConfigureInput() {}

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

void ezGameApplicationBase::Init_SetupDefaultResources()
{
  // continuously unload resources that are not in use anymore
  ezResourceManager::SetAutoFreeUnused(ezTime::Microseconds(100), ezTime::Seconds(10.0f));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezGameApplicationBase::Deinit_UnloadPlugins()
{
  ezPlugin::UnloadAllPlugins();
}

void ezGameApplicationBase::Deinit_ShutdownLogging()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
#endif
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplicationBaseInit);
