#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeViewWidget.moc.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

ezQtTextureCubeViewWidget::ezQtTextureCubeViewWidget(QWidget* pParent, ezQtTextureCubeAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig)
  : ezQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = EZ_DEFAULT_NEW(ezOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);
  m_pOrbitCameraContext->SetOrbitVolume(ezVec3(0), ezVec3(1.0f), ezVec3(-1, 0, 0));

  m_InputContexts.PushBack(m_pOrbitCameraContext);
}

ezQtTextureCubeViewWidget::~ezQtTextureCubeViewWidget()
{
  EZ_DEFAULT_DELETE(m_pOrbitCameraContext);
}
