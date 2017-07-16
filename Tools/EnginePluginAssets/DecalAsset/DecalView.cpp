#include <PCH.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Pipeline/View.h>

ezDecalViewContext::ezDecalViewContext(ezDecalContext* pDecalContext)
  : ezEngineProcessViewContext(pDecalContext)
{
  m_pDecalContext = pDecalContext;
}

ezDecalViewContext::~ezDecalViewContext()
{

}

ezViewHandle ezDecalViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Decal Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
