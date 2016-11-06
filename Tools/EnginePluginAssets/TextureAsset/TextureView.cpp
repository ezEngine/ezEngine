#include <PCH.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>
#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/View.h>

ezTextureViewContext::ezTextureViewContext(ezTextureContext* pContext)
  : ezEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
  m_pView = nullptr;
}

ezTextureViewContext::~ezTextureViewContext()
{
  ezRenderLoop::DeleteView(m_pView);

  if (GetEditorWindow().m_hWnd != 0)
  {
    static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->RemoveWindow(&GetEditorWindow());
  }
}

ezView* ezTextureViewContext::CreateView()
{
  ezView* pView = ezRenderLoop::CreateView("Texture Editor - View");

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetLogicCamera(&m_Camera);
  return pView;
}
