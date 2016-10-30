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

ezQtScenegraphPanel::ezQtScenegraphPanel(QWidget* pParent, ezSceneDocument* pDocument)
  : ezQtDocumentPanel(pParent)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("Scenegraph");

  m_pDocument = pDocument;

  m_pMainWidget = new QWidget(this);
  m_pMainWidget->setLayout(new QVBoxLayout(this));
  m_pMainWidget->setContentsMargins(0, 0, 0, 0);
  m_pMainWidget->layout()->setContentsMargins(0, 0, 0, 0);

  m_pFilterWidget = new ezQtSearchWidget(this);
  connect(m_pFilterWidget, &ezQtSearchWidget::textChanged, this, &ezQtScenegraphPanel::OnFilterTextChanged);

  m_pMainWidget->layout()->addWidget(m_pFilterWidget);

  m_pTreeWidget = new ezQtDocumentTreeView(this, pDocument, ezGetStaticRTTI<ezGameObject>(), "Children", std::unique_ptr<ezQtDocumentTreeModel>(new ezQtScenegraphModel(pDocument)));
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pMainWidget->layout()->addWidget(m_pTreeWidget);

  setWidget(m_pMainWidget);

  m_pDocument->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezQtScenegraphPanel::DocumentSceneEventHandler, this));

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(m_pTreeWidget, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnItemDoubleClicked(const QModelIndex&))) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr, "signal/slot connection failed");

}

ezQtScenegraphPanel::~ezQtScenegraphPanel()
{
  m_pDocument->m_SceneEvents.RemoveEventHandler(ezMakeDelegate(&ezQtScenegraphPanel::DocumentSceneEventHandler, this));

}

void ezQtScenegraphPanel::RegisterActions()
{
  ezActionMapManager::RegisterActionMap("ScenegraphContextMenu");

  ezSelectionActions::MapContextMenuActions("ScenegraphContextMenu", "");
  ezEditActions::MapContextMenuActions("ScenegraphContextMenu", "");
}

void ezQtScenegraphPanel::DocumentSceneEventHandler(const ezSceneDocumentEvent& e)
{
  switch (e.m_Type)
  {
  case ezSceneDocumentEvent::Type::ShowSelectionInScenegraph:
    {
      m_pTreeWidget->EnsureLastSelectedItemVisible();
    }
    break;
  }

}

void ezQtScenegraphPanel::OnItemDoubleClicked(const QModelIndex&)
{
  m_pDocument->TriggerFocusOnSelection(true);
}

void ezQtScenegraphPanel::OnRequestContextMenu(QPoint pos)
{
  ezQtMenuActionMapView menu(nullptr);

  ezActionContext context;
  context.m_sMapping = "ScenegraphContextMenu";
  context.m_pDocument = m_pDocument;
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));

}

void ezQtScenegraphPanel::OnFilterTextChanged(const QString& text)
{
  m_pTreeWidget->GetProxyFilterModel()->SetFilterText(text);
}




