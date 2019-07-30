#include <GameEnginePCH.h>

#include <GameEngine/GameApplication/GameApplicationBase.h>

#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Core/ResourceManager/ResourceManager.h>
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

void ezGameApplicationBase::Init_FileSystem_SetDataDirFactories()
{
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::ArchiveType::Factory);
}

void ezGameApplicationBase::Init_ConfigureAssetManagement() {}

void ezGameApplicationBase::Init_LoadRequiredPlugins()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezPlugin::LoadPlugin("XBoxControllerPlugin");
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

void ezGameApplicationBase::Init_SetupDefaultResources() {}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezGameApplicationBase::Deinit_UnloadPlugins()
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
    EZ_VERIFY(ezPlugin::UnloadPlugin(it, &iRefCount).Succeeded(), "Failed to unload plugin '{0}'", s);

    if (iRefCount == 0)
      ToUnload.PopBack();
  }
}

void ezGameApplicationBase::Deinit_ShutdownLogging()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
#endif
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplicationBaseInit);
