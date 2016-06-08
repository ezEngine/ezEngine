#include "Main.h"
#include "GameState.h"
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
#include <GameFoundation/GameApplication/GameApplication.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(SimpleMeshRendererGameState, 1, ezRTTIDefaultAllocator<SimpleMeshRendererGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE

SimpleMeshRendererGameState::SimpleMeshRendererGameState()
{
}

float SimpleMeshRendererGameState::CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  return 1.0f;
}

void SimpleMeshRendererGameState::OnActivation(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("SimpleMeshRendererGameState::Activate");

  ezGameState::OnActivation(pWorld);

  CreateGameLevel();
}

void SimpleMeshRendererGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("SimpleMeshRendererGameState::Deactivate");

  DestroyGameLevel();

  ezGameState::OnDeactivation();
}

EZ_APPLICATION_ENTRY_POINT(ezGameApplication, "SimpleMeshRenderer", ezGameApplicationType::StandAlone, "Data/Samples/SimpleMeshRenderer");
