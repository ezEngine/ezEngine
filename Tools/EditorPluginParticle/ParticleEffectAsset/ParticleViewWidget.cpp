#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleViewWidget.moc.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

ezQtParticleViewWidget::ezQtParticleViewWidget(QWidget* pParent, ezQtParticleEffectAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig)
  : ezQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = EZ_DEFAULT_NEW(ezOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);

  m_InputContexts.PushBack(m_pOrbitCameraContext);
}

ezQtParticleViewWidget::~ezQtParticleViewWidget()
{
  EZ_DEFAULT_DELETE(m_pOrbitCameraContext);
}
