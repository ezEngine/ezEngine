#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshViewWidget.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

ezQtMeshViewWidget::ezQtMeshViewWidget(QWidget* pParent, ezQtMeshAssetDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig)
  : ezQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = EZ_DEFAULT_NEW(ezOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);
  m_pOrbitCameraContext->SetOrbitVolume(ezVec3(0, 0, 1), ezVec3(10.0f), ezVec3(-5, 1, 2), true);

  m_InputContexts.PushBack(m_pOrbitCameraContext.Borrow());
}

ezQtMeshViewWidget::~ezQtMeshViewWidget()
{
}
