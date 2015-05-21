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

#include <RendererGL/Device/DeviceGL.h>
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

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory("");

  ezStringBuilder sReadDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sReadDir.AppendPath("../../Shared/Samples/SimpleMeshRenderer/");

  ezStringBuilder sObjectDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sObjectDir.AppendPath("../../Shared/FreeContent/");

  ezStringBuilder sBaseDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sBaseDir.AppendPath("../../Shared/Data/");

  // setup the 'asset management system'
  {
    // which redirection table to search
    ezDataDirectory::FolderType::s_sRedirectionFile = "AssetCache/LookupTable.ezAsset";
    // which platform assets to use
    ezDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/PC/";
  }

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory(sBaseDir.GetData(), ezFileSystem::ReadOnly, "Shared");
  ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::AllowWrites, "Game");
  ezFileSystem::AddDataDirectory(sObjectDir.GetData(), ezFileSystem::ReadOnly, "Object");

  m_pWindow = EZ_DEFAULT_NEW(GameWindow);
  ezGALSwapChainHandle hSwapChain = GetApplication()->AddWindow(m_pWindow);

  // Map the input keys to actions
  SetupInput();

  const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(hSwapChain);
  CreateGameLevelAndRenderPipeline(pSwapChain->GetRenderTargetViewConfig());
}

void GameState::Deactivate()
{
  EZ_LOG_BLOCK("GameState::Deactivate");

  DestroyGameLevel();

  EZ_DEFAULT_DELETE(m_pWindow);
}

EZ_CONSOLEAPP_ENTRY_POINT(ezGameApplication, *EZ_DEFAULT_NEW(GameState));
