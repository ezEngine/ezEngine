#include <GameFoundation/PCH.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <GameFoundation/GameApplication/InputConfig.h>
#include <Foundation/IO/FileSystem/FileReader.h>


namespace
{
  const char* g_szInputSet = "GameApp";
  const char* g_szCloseAppAction = "CloseApp";
  const char* g_szShowConsole = "ShowConsole";
  const char* g_szShowFpsAction = "ShowFps";
  const char* g_szReloadResourcesAction = "ReloadResources";
  const char* g_szCaptureProfilingAction = "CaptureProfiling";
}

void ezGameApplication::DoConfigureInput(bool bReinitialize)
{
  ezInputActionConfig config;

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
  ezInputManager::SetInputActionConfig(g_szInputSet, g_szCloseAppAction, config, true);

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyTilde;
  ezInputManager::SetInputActionConfig(g_szInputSet, g_szShowConsole, config, true);

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF2;
  ezInputManager::SetInputActionConfig(g_szInputSet, g_szShowFpsAction, config, true);

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF5;
  ezInputManager::SetInputActionConfig(g_szInputSet, g_szReloadResourcesAction, config, true);

  config.m_sInputSlotTrigger[0] = ezInputSlot_KeyF8;
  ezInputManager::SetInputActionConfig(g_szInputSet, g_szCaptureProfilingAction, config, true);

  {
    ezFileReader file;
    if ( file.Open( "InputConfig.json" ).Succeeded() )
    {
      ezHybridArray<ezGameAppInputConfig, 32> InputActions;

      ezGameAppInputConfig::ReadFromJson( file, InputActions );
      ezGameAppInputConfig::ApplyAll( InputActions );
    }

  }
}


void ezGameApplication::ProcessApplicationInput()
{
  if (ezInputManager::GetInputActionState(g_szInputSet, g_szCloseAppAction) == ezKeyState::Pressed)
  {
    RequestQuit();
  }

  if (ezInputManager::GetInputActionState(g_szInputSet, g_szShowConsole) == ezKeyState::Pressed)
  {
    m_bShowConsole = !m_bShowConsole;
  }

  if (ezInputManager::GetInputActionState(g_szInputSet, g_szShowFpsAction) == ezKeyState::Pressed)
  {
    m_bShowFps = !m_bShowFps;
  }

  if (ezInputManager::GetInputActionState(g_szInputSet, g_szReloadResourcesAction) == ezKeyState::Pressed)
  {
    ezResourceManager::ReloadAllResources();
  }

  if (ezInputManager::GetInputActionState(g_szInputSet, g_szCaptureProfilingAction) == ezKeyState::Pressed)
  {
    ezFileWriter fileWriter;
    if (fileWriter.Open("profiling.json") == EZ_SUCCESS)
    {
      ezProfilingSystem::Capture(fileWriter);
      ezLog::Info("Profiling capture saved to '%s'.", fileWriter.GetFilePathAbsolute().GetData());
    }
    else
    {
      ezLog::Error("Could not write profiling capture to '%s'.", fileWriter.GetFilePathAbsolute().GetData());
    }
  }
}



void ezGameApplication::UpdateInput()
{
  ezInputManager::Update(ezClock::GetGlobalClock()->GetTimeDiff());

  ProcessApplicationInput();

  if (m_pConsole)
  {
    m_pConsole->DoDefaultInputHandling(m_bShowConsole);
  }

  if (!m_bShowConsole || !m_pConsole)
  {
    for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
    {
      if (m_GameStates[i].m_pState)
      {
        m_GameStates[i].m_pState->ProcessInput();
      }
    }
  }
}