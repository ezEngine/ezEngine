#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshViewWidget.moc.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

ezQtCollisionMeshViewWidget::ezQtCollisionMeshViewWidget(QWidget* pParent, ezQtCollisionMeshAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig)
  : ezQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = EZ_DEFAULT_NEW(ezOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);
  m_pOrbitCameraContext->SetOrbitVolume(ezVec3(0, 0, 1), ezVec3(10.0f), ezVec3(-5, 1, 2));

  m_InputContexts.PushBack(m_pOrbitCameraContext);
}

ezQtCollisionMeshViewWidget::~ezQtCollisionMeshViewWidget()
{
  EZ_DEFAULT_DELETE(m_pOrbitCameraContext);
}
