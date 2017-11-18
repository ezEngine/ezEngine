#include <PCH.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <Core/World/GameObject.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <Actions/SelectionActions.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <QSortFilterProxyModel>
#include <QBoxLayout>
#include <QMenu>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>

ezQtScenegraphPanel::ezQtScenegraphPanel(QWidget* pParent, ezSceneDocument* pDocument)
  : ezQtGameObjectPanel(pParent, pDocument, "EditorPluginScene_ScenegraphContextMenu", std::unique_ptr<ezQtDocumentTreeModel>(new ezQtScenegraphModel(pDocument)))
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("Scenegraph");
  m_pSceneDocument = pDocument;
}

ezQtScenegraphPanel::~ezQtScenegraphPanel()
{
}

