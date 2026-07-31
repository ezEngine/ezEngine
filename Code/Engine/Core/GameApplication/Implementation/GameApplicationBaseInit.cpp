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
#include <Foundation/Logging/TraceWriter.h>
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
  ezGlobalLog::RemoveLogWriter(m_LogToTracingID);

  if (!opt_DisableConsoleOutput.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    m_LogToConsoleID = ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  }

  m_LogToVsID = ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  m_LogToTracingID = ezGlobalLog::AddLogWriter(ezLogWriter::Tracing::LogMessageHandler);
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

  {
    // the default config path is ":project/...", which would require the project data directory to be mounted already,
    // but that only happens further below, so the path has to be resolved to an absolute one instead
    // (the ":" data directory above is what makes absolute paths readable)
    ezStringBuilder sConfigFile;
    ezFileSystem::ResolveSpecialDirectory(GetProjectDataDirectoryPath(), sConfigFile).IgnoreResult();
    sConfigFile.AppendPath("RuntimeConfigs/DataDirectories.ddl");

    ezApplicationFileSystemConfig appFileSystemConfig;
    appFileSystemConfig.Load(sConfigFile);

    // get rid of duplicates that we already hard-coded above
    for (ezUInt32 i = appFileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
    {
      const ezString name = appFileSystemConfig.m_DataDirs[i - 1].m_sRootName;
      if (name.IsEqual_NoCase(":") || name.IsEqual_NoCase("bin") || name.IsEqual_NoCase("shadercache") || name.IsEqual_NoCase("appdata") || name.IsEqual_NoCase("base"))
      {
        appFileSystemConfig.m_DataDirs.RemoveAtAndCopy(i - 1);
      }
    }

    // ":project/" is deliberately NOT mounted before the config is applied, because its position *within* the
    // config matters. Data directories are searched back to front, and the editor lists the data directories of
    // the active plugin bundles before the project, so that a project can override plugin provided files
    // (e.g. ship its own version of a particle shader). Mounting the project up front would put every
    // directory from the config, including those, above it again.
    // The config entry is only patched up, so that the (virtual) GetProjectDataDirectoryPath() still decides
    // where the project is, and so that it stays read-only, which is what a shipped game wants.
    ezUInt32 uiProjectIdx = ezInvalidIndex;
    for (ezUInt32 i = 0; i < appFileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      const ezString name = appFileSystemConfig.m_DataDirs[i].m_sRootName;
      if (name.IsEqual_NoCase("project"))
      {
        uiProjectIdx = i;
        break;
      }
    }

    if (uiProjectIdx == ezInvalidIndex)
    {
      // no config file at all, or one that doesn't mention the project directory
      ezApplicationFileSystemConfig::DataDirConfig cfg;
      cfg.m_sRootName = "project";
      cfg.m_bHardCodedDependency = true;

      uiProjectIdx = appFileSystemConfig.m_DataDirs.GetCount();
      appFileSystemConfig.m_DataDirs.PushBack(cfg);
    }

    appFileSystemConfig.m_DataDirs[uiProjectIdx].m_sDataDirSpecialPath = GetProjectDataDirectoryPath();
    appFileSystemConfig.m_DataDirs[uiProjectIdx].m_bWritable = false;

    appFileSystemConfig.Apply();
  }

  {
    // We need the file system before we can start the html logger.

    ezGlobalLog::RemoveLogWriter(m_LogToHTML);
    ezStringBuilder sLogFile;
    sLogFile.SetFormat(":appdata/Log.htm");
    m_LogHTML.BeginLog(sLogFile, GetApplicationName());

    m_LogToHTML = ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezLogWriter::HTML::LogMessageHandler, &m_LogHTML));
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
  ezGlobalLog::RemoveLogWriter(m_LogToTracingID);
#endif

  ezGlobalLog::RemoveLogWriter(m_LogToHTML);
  m_LogHTML.EndLog();
}
