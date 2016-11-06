#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/TextureAsset/TextureViewWidget.moc.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

ezQtTextureViewWidget::ezQtTextureViewWidget(QWidget* pParent, ezQtTextureAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig)
  : ezQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = EZ_DEFAULT_NEW(ezOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);

  m_InputContexts.PushBack(m_pOrbitCameraContext);
}

ezQtTextureViewWidget::~ezQtTextureViewWidget()
{
  EZ_DEFAULT_DELETE(m_pOrbitCameraContext);
}
