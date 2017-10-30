#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/DecalAsset/DecalViewWidget.moc.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetWindow.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

ezQtDecalViewWidget::ezQtDecalViewWidget(QWidget* pParent, ezQtDecalAssetDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig)
  : ezQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = EZ_DEFAULT_NEW(ezOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);
  m_pOrbitCameraContext->SetOrbitVolume(ezVec3(0), ezVec3(0.0f), ezVec3(-2, 0, 0));

  m_InputContexts.PushBack(m_pOrbitCameraContext.Borrow());
}

ezQtDecalViewWidget::~ezQtDecalViewWidget()
{
}
