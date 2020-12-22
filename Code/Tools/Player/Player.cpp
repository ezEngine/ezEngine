#include <Player/Player.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Animation/RotorComponent.h>
#include <GameEngine/Animation/SliderComponent.h>
#include <GameEngine/Gameplay/InputComponent.h>
#include <GameEngine/Gameplay/SpawnComponent.h>
#include <GameEngine/Gameplay/TimedDeathComponent.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>

ezCommandLineOptionPath opt_Scene("_Player", "-scene", "Path to a scene file", "");

ezPlayerApplication::ezPlayerApplication()
  : ezGameApplication("ezPlayer", nullptr)
{
  m_pWorld = nullptr;
}

ezResult ezPlayerApplication::BeforeCoreSystemsStartup()
{
  {
    // since this is a GUI application (not a Console app), printf has no effect
    // therefore we have to show the command line options with a message box

    ezStringBuilder cmdHelp;
    if (ezCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, ezCommandLineOption::LogAvailableModes::IfHelpRequested))
    {
      ezLog::OsMessageBox(cmdHelp);
      return EZ_FAILURE;
    }
  }

  ezStartup::AddApplicationTag("player");

  EZ_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  //#TODO: We can't specify command line arguments on many platforms so the scene file should be defined by ezFileserve.
  // For now it's hardcoded and needs to be compiled in and ezFileserve started with the project special dir set, e.g.:
  // -specialdirs project ".../ezEngine/Data/Samples/Testing Chambers
#if EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  if (m_sSceneFile.IsEmpty())
  {
    m_sSceneFile = ">project/AssetCache/Common/Scenes/Surfaces.ezObjectGraph";
    m_sAppProjectPath = ">project";
  }
  else
#else
  m_sSceneFile = opt_Scene.GetOptionValue(ezCommandLineOption::LogMode::Always);
  EZ_ASSERT_ALWAYS(!m_sSceneFile.IsEmpty(), "Scene file has not been specified. Use the -scene command followed by a full path to the ezBinaryScene file");
#endif
  {
    ezStringBuilder projectPath;
    if (ezFileSystem::FindFolderWithSubPath(projectPath, m_sSceneFile, "ezProject").Succeeded())
    {
      m_sAppProjectPath = projectPath;
    }
  }

  EZ_ASSERT_ALWAYS(!m_sAppProjectPath.IsEmpty(), "No project directory could be found for scene file '{0}'", m_sSceneFile);

  return EZ_SUCCESS;
}


void ezPlayerApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  ezStartup::StartupHighLevelSystems();

  SetupLevel();

  ActivateGameState(m_pWorld.Borrow()).IgnoreResult();
}

void ezPlayerApplication::BeforeHighLevelSystemsShutdown()
{
  SUPER::BeforeHighLevelSystemsShutdown();

  m_pWorld = nullptr;
}

void ezPlayerApplication::SetupLevel()
{
  EZ_LOG_BLOCK("SetupLevel", m_sSceneFile.GetData());

  ezStringBuilder sScenePath(m_sSceneFile);
  ezString sSceneFile = sScenePath.GetFileName();

  ezWorldDesc desc(sSceneFile);
  m_pWorld = EZ_DEFAULT_NEW(ezWorld, desc);

  EZ_LOCK(m_pWorld->GetWriteMarker());

  {
    ezFileReader file;
    if (file.Open(m_sSceneFile).Succeeded())
    {
      // File Header
      {
        ezAssetFileHeader header;
        header.Read(file).IgnoreResult();

        char szSceneTag[16];
        file.ReadBytes(szSceneTag, sizeof(char) * 16);

        EZ_ASSERT_RELEASE(ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16), "The given file is not a valid scene file");
      }

      ezWorldReader reader;
      reader.ReadWorldDescription(file).IgnoreResult();
      reader.InstantiateWorld(*m_pWorld, nullptr);
      // reader.InstantiatePrefab(*m_pWorld, ezVec3(0, 2, 0), ezQuat::IdentityQuaternion(), ezVec3(0.1f));
    }
    else
    {
      ezLog::Error("Could not read level '{0}'", m_sSceneFile);
    }
  }
}


EZ_APPLICATION_ENTRY_POINT(ezPlayerApplication);
