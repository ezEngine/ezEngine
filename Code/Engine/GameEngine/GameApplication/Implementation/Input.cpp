#include <PCH.h>

#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Time/Clock.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/Console/Console.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <System/Window/Window.h>

extern ezCVarBool CVarShowFPS;

namespace
{
  const char* s_szInputSet = "GameApp";
  const char* s_szCloseAppAction = "CloseApp";
  const char* s_szShowConsole = "ShowConsole";
  const char* s_szShowFpsAction = "ShowFps";
  const char* s_szReloadResourcesAction = "ReloadResources";
  const char* s_szCaptureProfilingAction = "CaptureProfiling";
  const char* s_szTakeScreenshot = "TakeScreenshot";
} // namespace

void ezGameApplication::ProcessApplicationInput()
{
  // the show console command must be in the "Console" input set, because we are using that for exclusive input when the console is open
  if (ezInputManager::GetInputActionState("Console", s_szShowConsole) == ezKeyState::Pressed)
  {
    m_bShowConsole = !m_bShowConsole;

    if (m_bShowConsole)
      ezInputManager::SetExclusiveInputSet("Console");
    else
      ezInputManager::SetExclusiveInputSet("");
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szShowFpsAction) == ezKeyState::Pressed)
  {
    CVarShowFPS = !CVarShowFPS;
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szReloadResourcesAction) == ezKeyState::Pressed)
  {
    ezResourceManager::ReloadAllResources(false);
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szTakeScreenshot) == ezKeyState::Pressed)
  {
    TakeScreenshot();
  }

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szCaptureProfilingAction) == ezKeyState::Pressed)
  {
    TakeProfilingCapture();
  }

  if (m_bShowConsole && m_pConsole)
    return;

  if (ezInputManager::GetInputActionState(s_szInputSet, s_szCloseAppAction) == ezKeyState::Pressed)
  {
    RequestQuit();
  }
}

void ezGameApplication::UpdateInput()
{
  ezInputManager::Update(ezClock::GetGlobalClock()->GetTimeDiff());

  ProcessApplicationInput();

  if (m_pConsole)
  {
    m_pConsole->DoDefaultInputHandling(m_bShowConsole);

    if (m_bShowConsole)
      return;
  }

  if (m_pGameState)
  {
    m_pGameState->ProcessInput();
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_Input);
