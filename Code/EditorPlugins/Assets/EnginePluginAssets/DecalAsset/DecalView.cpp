#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezDecalViewContext::ezDecalViewContext(ezDecalContext* pDecalContext)
  : ezEngineProcessViewContext(pDecalContext)
{
  m_pDecalContext = pDecalContext;
}

ezDecalViewContext::~ezDecalViewContext() = default;

ezViewHandle ezDecalViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Decal Editor - View", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
