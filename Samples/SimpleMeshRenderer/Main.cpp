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

GameState::GameState()
{
  m_pWindow = nullptr;
  m_pRenderPipeline = nullptr;
  m_pWorld = nullptr;
  m_pView = nullptr;
}

void GameState::Activate()
{
  EZ_LOG_BLOCK("GameState::Activate");

  ezStringBuilder sBaseDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sBaseDir.AppendPath("../../Shared/Data/");

  ezStringBuilder sSharedDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sSharedDir.AppendPath("../../Shared/FreeContent/");

  ezStringBuilder sProjectDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sProjectDir.AppendPath("../../Shared/Samples/SimpleMeshRenderer");

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory("");
  ezFileSystem::AddDataDirectory(sBaseDir.GetData(), ezFileSystem::ReadOnly, "Base");
  ezFileSystem::AddDataDirectory(sSharedDir.GetData(), ezFileSystem::ReadOnly, "Shared");
  ezFileSystem::AddDataDirectory(sProjectDir.GetData(), ezFileSystem::AllowWrites, "Project");

  GetApplication()->SetupProject(sProjectDir);

  m_pWindow = EZ_DEFAULT_NEW(GameWindow);
  ezGALSwapChainHandle hSwapChain = GetApplication()->AddWindow(m_pWindow);

  // Map the input keys to actions
  SetupInput();

  const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(hSwapChain);
  CreateGameLevelAndRenderPipeline(pSwapChain->GetBackBufferRenderTargetView(), pSwapChain->GetDepthStencilTargetView());
}

void GameState::Deactivate()
{
  EZ_LOG_BLOCK("GameState::Deactivate");

  DestroyGameLevel();

  EZ_DEFAULT_DELETE(m_pWindow);
}

EZ_CONSOLEAPP_ENTRY_POINT(ezGameApplication, *EZ_DEFAULT_NEW(GameState));
