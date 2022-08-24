#include <Player/Player.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Input/InputManager.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Lock.h>
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

ezCommandLineOptionString opt_Project("_Player", "-project", "Path to the project folder.\nUsually an absolute path, though relative paths will work for projects that are located inside the EZ SDK directory.", "");
ezCommandLineOptionString opt_Scene("_Player", "-scene", "Path to a scene file.\nUsually given relative to the corresponding project data directory where it resides, but can also be given as an absolute path.", "");

ezPlayerApplication::ezPlayerApplication()
  : ezGameApplication("ezPlayer", nullptr) // we don't have a fixed project path in this app, so we need to pass that in a bit later
{
}

ezResult ezPlayerApplication::BeforeCoreSystemsStartup()
{
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

  m_State = State::Ok;

  if (DetermineProjectPath().Succeeded())
  {
    if (DetermineScenePath().Failed())
    {
      m_State = State::NoScene;
      m_Menu = Menu::SceneSelection;
    }
  }

  return EZ_SUCCESS;
}


void ezPlayerApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  ezStartup::StartupHighLevelSystems();

  // create the ezWorld into which we load all levels
  ezWorldDesc desc("MainWorld");
  m_pWorld = EZ_DEFAULT_NEW(ezWorld, desc);

  if (m_State == State::Ok)
  {
    if (LoadScene(m_sSceneFile).Failed())
    {
      m_State = State::BadScene;
      m_Menu = Menu::None;
      SetReturnCode(2);
    }
  }

  if (GetActiveGameState() == nullptr)
  {
    // if no scene was loaded (yet), still create a game-state, because that one creates the app's window
    // otherwise we won't see anything and can't interact with the menu
    ActivateGameState(m_pWorld.Borrow()).IgnoreResult();
  }
}

void ezPlayerApplication::BeforeHighLevelSystemsShutdown()
{
  SUPER::BeforeHighLevelSystemsShutdown();

  m_pWorld.Clear();
}

void ezPlayerApplication::Run_InputUpdate()
{
  SUPER::Run_InputUpdate();

  if (GetActiveGameState() && GetActiveGameState()->WasQuitRequested())
  {
    RequestQuit();
  }

  if (ezStringUtils::IsNullOrEmpty(ezInputManager::GetExclusiveInputSet()) ||
      ezStringUtils::IsEqual(ezInputManager::GetExclusiveInputSet(), "ezPlayer"))
  {
    if (DisplayMenu())
    {
      // prevents the currently active scene from getting any input
      ezInputManager::SetExclusiveInputSet("ezPlayer");
    }
    else
    {
      // allows the active scene to retrieve input again
      ezInputManager::SetExclusiveInputSet("");
    }
  }
}

ezResult ezPlayerApplication::DetermineProjectPath()
{
  ezStringBuilder sProjectPath = opt_Project.GetOptionValue(ezCommandLineOption::LogMode::FirstTime);

#if EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // We can't specify command line arguments on many platforms so the project must be defined by ezFileserve.
  // ezFileserve must be started with the project special dir set. For example:
  // -specialdirs project ".../ezEngine/Data/Samples/Testing Chambers

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = ">project";
    return EZ_SUCCESS;
  }
#endif

  if (sProjectPath.IsEmpty())
  {
    const ezStringBuilder sScenePath = opt_Scene.GetOptionValue(ezCommandLineOption::LogMode::FirstTime);

    // project path is empty, need to extract it from the scene path

    if (!sScenePath.IsAbsolutePath())
    {
      // scene path is not absolute -> can't extract project path
      m_State = State::NoProject;
      m_sAppProjectPath = ezFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return EZ_FAILURE;
    }

    if (ezFileSystem::FindFolderWithSubPath(sProjectPath, sScenePath, "ezProject", "ezSdkRoot.txt").Failed())
    {
      // couldn't find the 'ezProject' file in any parent folder of the scene
      m_State = State::NoProject;
      m_sAppProjectPath = ezFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return EZ_FAILURE;
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
    m_State = State::NoProject;
    m_sAppProjectPath = ezFileSystem::GetSdkRootDirectory();
    SetReturnCode(1);
    return EZ_FAILURE;
  }

  // store it now, even if it fails, for error reporting
  m_sAppProjectPath = sProjectPath;

  if (!ezOSFile::ExistsDirectory(sProjectPath))
  {
    m_State = State::BadProject;
    SetReturnCode(1);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezPlayerApplication::DetermineScenePath()
{
  ezStringBuilder sScenePath = opt_Scene.GetOptionValue(ezCommandLineOption::LogMode::FirstTime);

#if EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // TODO: We can't specify command line arguments on many platforms so the scene file is currently hardcoded
  if (sScenePath.IsEmpty())
  {
    m_sSceneFile = "Scenes/Empty.ezScene";
    return EZ_SUCCESS;
  }
#endif

  sScenePath.MakeCleanPath();

  if (sScenePath.IsEmpty())
    return EZ_FAILURE;

  if (sScenePath.IsAbsolutePath())
  {
    // this is just to make the path shorter, when possible
    // but can fail if the scene is in another data directory
    sScenePath.MakeRelativeTo(m_sAppProjectPath).IgnoreResult();
  }

  m_sSceneFile = sScenePath;
  return EZ_SUCCESS;
}

ezResult ezPlayerApplication::LoadScene(const char* szFile)
{
  EZ_LOG_BLOCK("LoadScene", szFile);

  ezStringBuilder sSceneFile = szFile;

  if (sSceneFile.IsEmpty())
  {
    ezLog::Error("No scene file specified.");
    return EZ_FAILURE;
  }

  ezLog::Info("Loading scene '{}'.", szFile);

  if (sSceneFile.IsAbsolutePath())
  {
    // this can fail if the scene is in a different data directory than the project directory
    // shouldn't stop us from loading it anyway
    sSceneFile.MakeRelativeTo(m_sAppProjectPath).IgnoreResult();
  }

  if (sSceneFile.HasExtension("ezScene") || sSceneFile.HasExtension("ezPrefab"))
  {
    if (sSceneFile.IsRelativePath())
    {
      // if this is a path to the non-transformed source file, redirect it to the transformed file in the asset cache
      sSceneFile.Prepend("AssetCache/Common/");
      sSceneFile.ChangeFileExtension("ezObjectGraph");
    }
  }

  if (sSceneFile != szFile)
  {
    ezLog::Info("Redirecting scene file from '{}' to '{}'", szFile, sSceneFile);
  }

  EZ_SUCCEED_OR_RETURN(LoadObjectGraph(sSceneFile));

  // (re-)create the game-state
  // this is either custom game code, or the ezFallbackGameState
  // it is responsible for creating the main window, setting up the input devices
  // and adding high-level game logic
  {
    DeactivateGameState();
    ActivateGameState(m_pWorld.Borrow()).IgnoreResult();
  }

  ezLog::Success("Successfully loaded scene.");
  return EZ_SUCCESS;
}

ezResult ezPlayerApplication::LoadObjectGraph(const char* szFile)
{
  EZ_LOG_BLOCK("LoadObjectGraph", szFile);

  EZ_ASSERT_DEV(m_pWorld != nullptr, "ezWorld must be created before loading anything into it.");
  EZ_LOCK(m_pWorld->GetWriteMarker());

  // make sure the world is empty
  m_pWorld->Clear();

  ezFileReader file;

  if (file.Open(szFile).Failed())
  {
    ezLog::Error("Failed to open the file.");
    return EZ_FAILURE;
  }

  // Read and skip the asset file header
  {
    ezAssetFileHeader header;
    header.Read(file).AssertSuccess();

    char szSceneTag[16];
    file.ReadBytes(szSceneTag, sizeof(char) * 16);

    if (!ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16))
    {
      ezLog::Error("The given file isn't an object-graph file.");
      return EZ_FAILURE;
    }
  }

  ezWorldReader reader;
  if (reader.ReadWorldDescription(file).Failed())
  {
    ezLog::Error("Error reading world description.");
    return EZ_FAILURE;
  }

  reader.InstantiateWorld(*m_pWorld, nullptr);
  return EZ_SUCCESS;
}

void ezPlayerApplication::FindAvailableScenes()
{
  if (m_bCheckedForScenes)
    return;

  m_bCheckedForScenes = true;

  ezFileSystemIterator fsit;
  ezStringBuilder sScenePath;

  for (ezFileSystem::StartSearch(fsit, "", ezFileSystemIteratorFlags::ReportFilesRecursive);
       fsit.IsValid(); fsit.Next())
  {
    fsit.GetStats().GetFullPath(sScenePath);

    if (!sScenePath.HasExtension(".ezScene"))
      continue;

    sScenePath.MakeRelativeTo(fsit.GetCurrentSearchTerm()).AssertSuccess();

    m_AvailableScenes.PushBack(sScenePath);
  }
}

bool ezPlayerApplication::DisplayMenu()
{
  if (m_State == State::NoProject)
  {
    ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", "No project path provided.\n\nUse the command-line argument\n-project \"Path/To/ezProject\"\nto tell ezPlayer which project to load.\n\nWith the argument\n-scene \"Path/To/Scene.ezScene\"\nyou can also directly load a specific scene.\n\nPress ESC to quit.", ezColor::Red);

    return false;
  }

  if (m_State == State::BadProject)
  {
    ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("Invalid project path provided.\nThe given project directory does not exist:\n\n{}\n\nPress ESC to quit.", m_sAppProjectPath), ezColor::Red);

    return false;
  }

  if (ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftWin) == ezKeyState::Pressed || ezInputManager::GetInputSlotState(ezInputSlot_KeyRightWin) == ezKeyState::Pressed)
  {
    if (m_Menu == Menu::SceneSelection)
      m_Menu = Menu::None;
    else
      m_Menu = Menu::SceneSelection;
  }

  if (m_State == State::Ok && m_Menu == Menu::None)
    return false;

  ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("Project: '{}'", m_sAppProjectPath), ezColor::White);

  if (m_State == State::BadScene)
  {
    ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("Failed to load scene: '{}'", m_sSceneFile), ezColor::Red);
  }
  else
  {
    ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("Scene: '{}'", m_sSceneFile), ezColor::White);
  }

  if (m_Menu == Menu::SceneSelection)
  {
    FindAvailableScenes();

    ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", "\nSelect scene:\n", ezColor::White);

    for (ezUInt32 i = 0; i < m_AvailableScenes.GetCount(); ++i)
    {
      const auto& file = m_AvailableScenes[i];

      if (i == m_uiSelectedScene)
      {
        ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("> {} <", file), ezColor::Gold);
      }
      else
      {
        ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("  {}  ", file), ezColor::GhostWhite);
      }
    }

    ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", "\nPress 'Return' to load scene.\nPress the 'Windows' key to toggle this menu.", ezColor::White);

    if (ezInputManager::GetInputSlotState(ezInputSlot_KeyEscape) == ezKeyState::Pressed)
    {
      m_Menu = Menu::None;
    }
    else if (!m_AvailableScenes.IsEmpty())
    {
      if (ezInputManager::GetInputSlotState(ezInputSlot_KeyUp) == ezKeyState::Pressed)
      {
        if (m_uiSelectedScene == 0)
          m_uiSelectedScene = m_AvailableScenes.GetCount() - 1;
        else
          --m_uiSelectedScene;
      }

      if (ezInputManager::GetInputSlotState(ezInputSlot_KeyDown) == ezKeyState::Pressed)
      {
        if (m_uiSelectedScene == m_AvailableScenes.GetCount() - 1)
          m_uiSelectedScene = 0;
        else
          ++m_uiSelectedScene;
      }

      if (ezInputManager::GetInputSlotState(ezInputSlot_KeyReturn) == ezKeyState::Pressed || ezInputManager::GetInputSlotState(ezInputSlot_KeyNumpadEnter) == ezKeyState::Pressed)
      {
        m_sSceneFile = m_AvailableScenes[m_uiSelectedScene];

        if (LoadScene(m_AvailableScenes[m_uiSelectedScene]).Succeeded())
        {
          m_State = State::Ok;
          m_Menu = Menu::None;
        }
        else
        {
          m_State = State::BadScene;
          m_Menu = Menu::SceneSelection;
        }
      }

      return true;
    }
  }

  return false;
}
