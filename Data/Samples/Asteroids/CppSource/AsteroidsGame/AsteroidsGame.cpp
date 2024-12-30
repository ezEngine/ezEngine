#include <AsteroidsGame/AsteroidsGame.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>

// this injects the C++ main() function
EZ_APPLICATION_ENTRY_POINT(AsteroidsGame);

AsteroidsGame::AsteroidsGame()
  : ezGameApplication("Asteroids", nullptr)
{
}

ezResult AsteroidsGame::TryProjectFolder(ezStringView sPath)
{
  ezStringBuilder sProjDir = sPath;
  sProjDir.MakeCleanPath();

  ezStringBuilder sProjFile;
  sProjFile.SetPath(sProjDir, "ezProject");

  if (sProjFile.IsAbsolutePath() && ezOSFile::ExistsFile(sProjFile))
  {
    m_sAppProjectPath = sProjDir;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void AsteroidsGame::DetermineProjectPath()
{
  // IMPORTANT!
  //
  // The project path has to be set for the ezGameApplication to know where the main 'project' data directory is.
  // Without it, nothing will work (the game plugin won't be loaded etc).
  //
  // The path can be relative to the '>SDK' directory (the root folder where EZ is located).
  // It may also be absolute (though this isn't portable across machines).
  // Or it can be relative to ezOSFile::GetApplicationDirectory() (where the Game.exe is).
  //
  // If your project is inside the EZ directory, use a relative path from there.
  // If it is somewhere outside, you either need to use an absolute path or some other way to locate it.
  //
  // Note that in a final exported build the project folder is always merged with the EZ data folders into one package.

  // this path works for exported projects, because during export the project folder is always copied there
  ezStringBuilder sProjDir;
  if (ezFileSystem::ResolveSpecialDirectory(">sdk/Data/project", sProjDir).Succeeded())
  {
    if (TryProjectFolder(sProjDir).Succeeded())
      return;
  }

#ifdef GAME_PROJECT_FOLDER
  // this absolute path will only work on the machine where the game is compiled,
  // but it works for projects that are located outside the ezEngine folder
  if (TryProjectFolder(EZ_PP_STRINGIFY(GAME_PROJECT_FOLDER)).Succeeded())
    return;
#endif

  // in other cases, try this relative path
  m_sAppProjectPath = "Data/Samples/Asteroids";
}

ezUniquePtr<ezGameStateBase> AsteroidsGame::CreateGameState()
{
  // usually we should only have a single non-fallback gamestate which is automatically picked
  // but if necessary, we can override this here
  return SUPER::CreateGameState();
}

ezResult AsteroidsGame::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("game");

  EZ_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  DetermineProjectPath();

  return EZ_SUCCESS;
}

void AsteroidsGame::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  ezStartup::StartupHighLevelSystems();

  // we need a game state to do anything
  // if no custom game state is available, ezFallbackGameState will be used
  // the game state is also responsible for either creating a world, or loading it
  // the ezFallbackGameState inspects the command line to figure out which scene to load
  ActivateGameState(nullptr, {}, ezTransform::MakeIdentity());
}

void AsteroidsGame::Run_InputUpdate()
{
  SUPER::Run_InputUpdate();

  if (auto pGameState = GetActiveGameState())
  {
    // pass through the closing of the application
    if (pGameState->WasQuitRequested())
    {
      RequestApplicationQuit();
    }
  }
}
