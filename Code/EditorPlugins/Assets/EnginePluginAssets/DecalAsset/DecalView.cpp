#include <EnginePluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezDecalViewContext::ezDecalViewContext(ezDecalContext* pDecalContext)
    : ezEngineProcessViewContext(pDecalContext)
{
  m_pDecalContext = pDecalContext;
}

ezDecalViewContext::~ezDecalViewContext() {}

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
