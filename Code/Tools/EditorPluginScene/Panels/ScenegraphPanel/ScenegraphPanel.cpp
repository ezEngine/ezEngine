#include <EditorPluginScenePCH.h>

#include <Actions/SelectionActions.h>
#include <Core/World/GameObject.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QBoxLayout>
#include <QMenu>
#include <QSortFilterProxyModel>

namespace
{
  std::unique_ptr<ezQtDocumentTreeModel> CreateSceneTreeModel(ezSceneDocument* pDocument)
  {
    std::unique_ptr<ezQtDocumentTreeModel> pModel(new ezQtScenegraphModel(pDocument));
    pModel->AddAdapter(new ezQtDummyAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezDocumentRoot>(), "Children"));
    pModel->AddAdapter(new ezQtGameObjectAdapter(pDocument));
    return std::move(pModel);
  }
}

ezQtScenegraphPanel::ezQtScenegraphPanel(QWidget* pParent, ezSceneDocument* pDocument)
    : ezQtGameObjectPanel(pParent, pDocument, "EditorPluginScene_ScenegraphContextMenu", CreateSceneTreeModel(pDocument))
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("Scenegraph");
  m_pSceneDocument = pDocument;
}

ezQtScenegraphPanel::~ezQtScenegraphPanel() {}
