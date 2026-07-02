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
  ezView* pView = CreateDefaultView("Decal Editor - View");
  return pView->GetHandle();
}
