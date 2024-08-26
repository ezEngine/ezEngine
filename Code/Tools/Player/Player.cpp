#include <Player/Player.h>

#include <Core/Input/InputManager.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Animation/RotorComponent.h>
#include <GameEngine/Animation/SliderComponent.h>
#include <GameEngine/Gameplay/InputComponent.h>
#include <GameEngine/Gameplay/SpawnComponent.h>
#include <GameEngine/Gameplay/TimedDeathComponent.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>

// this injects the main function
EZ_APPLICATION_ENTRY_POINT(ezPlayerApplication);

// these command line options may not all be directly used in ezPlayer, but the ezFallbackGameState reads those options to determine which scene to load
ezCommandLineOptionString opt_Project("_Player", "-project", "Path to the project folder.\nUsually an absolute path, though relative paths will work for projects that are located inside the EZ SDK directory.", "");
ezCommandLineOptionString opt_Scene("_Player", "-scene", "Path to a scene file.\nUsually given relative to the corresponding project data directory where it resides, but can also be given as an absolute path.", "");

ezPlayerApplication::ezPlayerApplication()
  : ezGameApplication("ezPlayer", nullptr) // we don't have a fixed project path in this app, so we need to pass that in a bit later
{
}

ezResult ezPlayerApplication::BeforeCoreSystemsStartup()
{
  // show the command line options, if help is requested
  {
    // since this is a GUI application (not a console app), printf has no effect
    // therefore we have to show the command line options with a message box

    ezStringBuilder cmdHelp;
    if (ezCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, ezCommandLineOption::LogAvailableModes::IfHelpRequested))
    {
      ezLog::OsMessageBox(cmdHelp);
      SetReturnCode(-1);
      return EZ_FAILURE;
    }
  }

  ezStartup::AddApplicationTag("player");

  EZ_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  DetermineProjectPath();

  return EZ_SUCCESS;
}


void ezPlayerApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  ezStartup::StartupHighLevelSystems();

  // we need a game state to do anything
  // if no custom game state is available, ezFallbackGameState will be used
  // the game state is also responsible for either creating a world, or loading it
  // the ezFallbackGameState inspects the command line to figure out which scene to load
  ActivateGameState(nullptr, {}, ezTransform::MakeIdentity());
}

void ezPlayerApplication::Run_InputUpdate()
{
  SUPER::Run_InputUpdate();

  if (auto pGameState = GetActiveGameState())
  {
    if (pGameState->WasQuitRequested())
    {
      RequestQuit();
    }
  }
}

void ezPlayerApplication::DetermineProjectPath()
{
  ezStringBuilder sProjectPath = opt_Project.GetOptionValue(ezCommandLineOption::LogMode::FirstTime);

#if EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // We can't specify command line arguments on many platforms so the project must be defined by ezFileserve.
  // ezFileserve must be started with the project special dir set. For example:
  // -specialdirs project ".../ezEngine/Data/Samples/Testing Chambers

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = ">project";
    return;
  }
#endif

  if (sProjectPath.IsEmpty())
  {
    const ezStringBuilder sScenePath = opt_Scene.GetOptionValue(ezCommandLineOption::LogMode::FirstTime);

    // project path is empty, need to extract it from the scene path

    if (!sScenePath.IsAbsolutePath())
    {
      // scene path is not absolute -> can't extract project path
      m_sAppProjectPath = ezFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }

    if (ezFileSystem::FindFolderWithSubPath(sProjectPath, sScenePath, "ezProject", "ezSdkRoot.txt").Failed())
    {
      // couldn't find the 'ezProject' file in any parent folder of the scene
      m_sAppProjectPath = ezFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }
  }
  else if (!ezPathUtils::IsAbsolutePath(sProjectPath))
  {
    // project path is not absolute, so must be relative to the SDK directory
    sProjectPath.Prepend(ezFileSystem::GetSdkRootDirectory(), "/");
  }

  sProjectPath.MakeCleanPath();
  sProjectPath.TrimWordEnd("/ezProject");

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = ezFileSystem::GetSdkRootDirectory();
    SetReturnCode(1);
    return;
  }

  // store it now, even if it fails, for error reporting
  m_sAppProjectPath = sProjectPath;

  if (!ezOSFile::ExistsDirectory(sProjectPath))
  {
    SetReturnCode(1);
    return;
  }
}
