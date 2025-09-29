#include <Core/CorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>

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
#include <Foundation/Platform/PlatformDesc.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Utilities/CommandLineOptions.h>

ezCommandLineOptionBool opt_DisableConsoleOutput("app", "-disableConsoleOutput", "Disables logging to the standard console window.", false);
ezCommandLineOptionInt opt_TelemetryPort("app", "-TelemetryPort", "The network port over which telemetry is sent.", ezTelemetry::s_uiPort);
ezCommandLineOptionString opt_Profile("app", "-profile", "The platform profile to use.", "Default");

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
  Init_ConfigureTags();
  Init_ConfigureCVars();
  Init_SetupGraphicsDevice();
  Init_SetupDefaultResources();
}

void ezGameApplicationBase::Init_PlatformProfile_SetPreferred()
{
  if (opt_Profile.IsOptionSpecified())
  {
    m_PlatformProfile.SetConfigName(opt_Profile.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified));
  }
  else
  {
    m_PlatformProfile.SetConfigName(ezPlatformDesc::GetThisPlatformDesc().GetName());

    const ezStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.GetConfigName(), ".ezProfile");

    if (!ezFileSystem::ExistsFile(sRuntimeProfileFile))
    {
      ezLog::Info("Platform profile '{}' doesn't exist, switching to 'Default'", m_PlatformProfile.GetConfigName());

      m_PlatformProfile.SetConfigName("Default");
    }
  }

  m_PlatformProfile.AddMissingConfigs();
}

void ezGameApplicationBase::BaseInit_ConfigureLogging()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  ezGlobalLog::RemoveLogWriter(m_LogToVsID);

  if (!opt_DisableConsoleOutput.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    m_LogToConsoleID = ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  }

  m_LogToVsID = ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
#endif
}

void ezGameApplicationBase::Init_ConfigureTelemetry()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezTelemetry::s_uiPort = static_cast<ezUInt16>(opt_TelemetryPort.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified));
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
  ezPlugin::InitializeStaticallyLinkedPlugins();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezPlugin::LoadPlugin("XBoxControllerPlugin", ezPluginLoadFlags::PluginIsOptional).IgnoreResult();
#endif
}

void ezGameApplicationBase::Init_FileSystem_ConfigureDataDirs()
{
  // ">appdir/" and ">user/" are built-in special directories
  // see ezFileSystem::ResolveSpecialDirectory

  const ezStringBuilder sUserDataPath(">user/", GetApplicationName());

  ezFileSystem::CreateDirectoryStructure(sUserDataPath).AssertSuccess();

  ezString writableBinRoot = ">appdir/";
  ezString shaderCacheRoot = ">sdk/Output/";

#if EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // On platforms where this is disabled, one can usually only write to the user directory
  // e.g. on mobile platforms
  writableBinRoot = sUserDataPath;
#endif

  ezFileSystem::CreateDirectoryStructure(shaderCacheRoot).IgnoreResult();

  // for absolute paths, read-only
  ezFileSystem::AddDataDirectory("", "GameApplicationBase", ":", ezDataDirUsage::ReadOnly).AssertSuccess();

  // ":bin/" : writing to the binary directory
  ezFileSystem::AddDataDirectory(writableBinRoot, "GameApplicationBase", "bin", ezDataDirUsage::AllowWrites).AssertSuccess();

  // ":shadercache/" for reading and writing shader files
#if EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  ezFileSystem::AddDataDirectory(shaderCacheRoot, "GameApplicationBase", "shadercache", ezDataDirUsage::ReadOnly).AssertSuccess();
#else
  ezFileSystem::AddDataDirectory(shaderCacheRoot, "GameApplicationBase", "shadercache", ezDataDirUsage::AllowWrites).AssertSuccess();
#endif

  // ":appdata/" for reading and writing app user data
  ezFileSystem::AddDataDirectory(sUserDataPath, "GameApplicationBase", "appdata", ezDataDirUsage::AllowWrites).AssertSuccess();

  // ":base/" for reading the core engine files
  ezFileSystem::AddDataDirectory(GetBaseDataDirectoryPath(), "GameApplicationBase", "base", ezDataDirUsage::ReadOnly).IgnoreResult();

  // ":project/" for reading the project specific files
  ezFileSystem::AddDataDirectory(GetProjectDataDirectoryPath(), "GameApplicationBase", "project", ezDataDirUsage::ReadOnly).IgnoreResult();

  // ":plugins/" for plugin specific data (optional, if it exists)
  {
    ezStringBuilder dir;
    ezFileSystem::ResolveSpecialDirectory(">sdk/Data/Plugins", dir).IgnoreResult();
    if (dir.IsAbsolutePath() && ezOSFile::ExistsDirectory(dir))
    {
      ezFileSystem::AddDataDirectory(">sdk/Data/Plugins", "GameApplicationBase", "plugins", ezDataDirUsage::ReadOnly).IgnoreResult();
    }
  }

  {
    ezApplicationFileSystemConfig appFileSystemConfig;
    appFileSystemConfig.Load();

    // get rid of duplicates that we already hard-coded above
    for (ezUInt32 i = appFileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
    {
      const ezString name = appFileSystemConfig.m_DataDirs[i - 1].m_sRootName;
      if (name.IsEqual_NoCase(":") || name.IsEqual_NoCase("bin") || name.IsEqual_NoCase("shadercache") || name.IsEqual_NoCase("appdata") || name.IsEqual_NoCase("base") || name.IsEqual_NoCase("project") || name.IsEqual_NoCase("plugins"))
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
  appPluginConfig.Apply();
}

void ezGameApplicationBase::Init_PlatformProfile_LoadForRuntime()
{
  const ezStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.GetConfigName(), ".ezProfile");
  m_PlatformProfile.AddMissingConfigs();

  m_PlatformProfile.LoadForRuntime(sRuntimeProfileFile).IgnoreResult();
}

void ezGameApplicationBase::Init_ConfigureTags()
{
  EZ_LOG_BLOCK("Reading Tags", "Tags.ddl");

  ezStringView sFile = ":project/RuntimeConfigs/Tags.ddl";

  ezFileReader file;
  if (file.Open(sFile).Failed())
  {
    ezLog::Dev("'{}' does not exist", sFile);
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
  ezResourceManager::SetAutoFreeUnused(ezTime::MakeFromMicroseconds(100), ezTime::MakeFromSeconds(10.0f));
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
#if EZ_DISABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  // during development, keep these loggers active
  ezGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  ezGlobalLog::RemoveLogWriter(m_LogToVsID);
#endif
}
