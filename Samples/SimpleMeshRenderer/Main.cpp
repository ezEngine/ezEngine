#include "Main.h"
#include "GameState.h"
#include "Window.h"
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Time/Clock.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Application/Config/ApplicationConfig.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderContext/RenderContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(SimpleMeshRendererGameState, 1, ezRTTIDefaultAllocator<SimpleMeshRendererGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

SimpleMeshRendererGameState::SimpleMeshRendererGameState()
{
  m_pWindow = nullptr;
  m_pRenderPipeline = nullptr;
  m_pWorld = nullptr;
  m_pView = nullptr;
}

ezGameStateCanHandleThis SimpleMeshRendererGameState::CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  return ezGameStateCanHandleThis::Yes;
}

void SimpleMeshRendererGameState::Activate(ezGameApplicationType AppType, ezWorld* pWorld)
{
  EZ_LOG_BLOCK("SimpleMeshRendererGameState::Activate");

  m_pWindow = EZ_DEFAULT_NEW(GameWindow);
  ezGALSwapChainHandle hSwapChain = GetApplication()->AddWindow(m_pWindow);

  // Map the input keys to actions
  SetupInput();

  const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(hSwapChain);
  CreateGameLevelAndRenderPipeline(pSwapChain->GetBackBufferRenderTargetView(), pSwapChain->GetDepthStencilTargetView());
}

void SimpleMeshRendererGameState::Deactivate()
{
  EZ_LOG_BLOCK("SimpleMeshRendererGameState::Deactivate");

  DestroyGameLevel();

  EZ_DEFAULT_DELETE(m_pWindow);
}

EZ_CONSOLEAPP_ENTRY_POINT(ezGameApplication, ezGameApplicationType::StandAlone, "Shared/Samples/SimpleMeshRenderer");
