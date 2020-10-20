#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

ezEditorEngineProcessAppUWP::ezEditorEngineProcessAppUWP() {}

ezEditorEngineProcessAppUWP::~ezEditorEngineProcessAppUWP() {}

ezViewHandle ezEditorEngineProcessAppUWP::CreateRemoteWindowAndView(ezCamera* pCamera)
{
  EZ_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  CreateRemoteWindow();

  return ezEditorEngineProcessApp::CreateRemoteWindowAndView(pCamera);
}

ezRenderPipelineResourceHandle ezEditorEngineProcessAppUWP::CreateDefaultMainRenderPipeline()
{
  return ezEditorEngineProcessApp::CreateDefaultMainRenderPipeline();
}

ezRenderPipelineResourceHandle ezEditorEngineProcessAppUWP::CreateDefaultDebugRenderPipeline()
{
  return CreateDefaultMainRenderPipeline();
}

