#include <PacManGame/PacManGame.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>

// this injects the main function
EZ_APPLICATION_ENTRY_POINT(ezPacManGame);

ezPacManGame::ezPacManGame()
  : ezGameApplication("PacMan", nullptr)
{
}

ezResult ezPacManGame::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("game");

  EZ_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  // IMPORTANT!
  //
  // This path has to be set for the ezGameApplication to know where the main 'project' directory is.
  // The path can be relative to the '>SDK' directory (the root folder where EZ is located).
  // It may also be absolute (though this isn't portable across machines).
  // Or it can be relative to ezOSFile::GetApplicationDirectory() (where the Game.exe is).
  //
  // If your project is inside the EZ directory, use a relative path from there.
  // If it is somewhere outside, you either need to use an absolute path or some other way to locate it.
  // 
  // Note that in a final exported build the project folder is always merged with the EZ data folders into one package.
  m_sAppProjectPath = "Data/Samples/PacMan";

  return EZ_SUCCESS;
}


void ezPacManGame::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  ezStartup::StartupHighLevelSystems();

  // we need a game state to do anything
  // if no custom game state is available, ezFallbackGameState will be used
  // the game state is also responsible for either creating a world, or loading it
  // the ezFallbackGameState inspects the command line to figure out which scene to load
  ActivateGameState(nullptr, {}, ezTransform::MakeIdentity());
}

ezUniquePtr<ezGameStateBase> ezPacManGame::CreateGameState()
{
  // usually we should only have a single non-fallback gamestate which is automatically picked
  // but if necessary, we can override this here
  return SUPER::CreateGameState();
}

void ezPacManGame::Run_InputUpdate()
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
