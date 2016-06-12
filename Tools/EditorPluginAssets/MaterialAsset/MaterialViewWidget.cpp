#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/MaterialAsset/MaterialViewWidget.moc.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>




ezQtMaterialViewWidget::ezQtMaterialViewWidget(QWidget* pParent, ezMaterialAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig)
  : ezQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);
}

ezQtMaterialViewWidget::~ezQtMaterialViewWidget()
{
}
