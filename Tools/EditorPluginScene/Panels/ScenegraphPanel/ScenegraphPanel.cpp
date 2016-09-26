#include <PCH.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <Core/World/GameObject.h>
#include <QMenu>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <Actions/SelectionActions.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <QBoxLayout>
#include <QSortFilterProxyModel>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>

ezScenegraphPanel::ezScenegraphPanel(QWidget* pParent, ezSceneDocument* pDocument)
  : ezDocumentPanel(pParent)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("Scenegraph");

  m_pDocument = pDocument;

  m_pMainWidget = new QWidget(this);
  m_pMainWidget->setLayout(new QVBoxLayout(this));
  m_pMainWidget->setContentsMargins(0, 0, 0, 0);
  m_pMainWidget->layout()->setContentsMargins(0, 0, 0, 0);

  m_pFilterWidget = new ezQtSearchWidget(this);
  connect(m_pFilterWidget, &ezQtSearchWidget::textChanged, this, &ezScenegraphPanel::OnFilterTextChanged);

  m_pMainWidget->layout()->addWidget(m_pFilterWidget);

  m_pTreeWidget = new ezQtDocumentTreeWidget(this, pDocument, ezGetStaticRTTI<ezGameObject>(), "Children", std::unique_ptr<ezQtDocumentTreeModel>(new ezQtScenegraphModel(pDocument)));
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pMainWidget->layout()->addWidget(m_pTreeWidget);

  setWidget(m_pMainWidget);

  m_pDocument->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezScenegraphPanel::DocumentSceneEventHandler, this));

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(m_pTreeWidget, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnItemDoubleClicked(const QModelIndex&))) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr, "signal/slot connection failed");

}

ezScenegraphPanel::~ezScenegraphPanel()
{
  m_pDocument->m_SceneEvents.RemoveEventHandler(ezMakeDelegate(&ezScenegraphPanel::DocumentSceneEventHandler, this));
  
}

void ezScenegraphPanel::RegisterActions()
{
  ezActionMapManager::RegisterActionMap("ScenegraphContextMenu");

  ezSelectionActions::MapContextMenuActions("ScenegraphContextMenu", "");
  ezEditActions::MapContextMenuActions("ScenegraphContextMenu", "");
}

void ezScenegraphPanel::DocumentSceneEventHandler(const ezSceneDocumentEvent& e)
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

void ezScenegraphPanel::OnItemDoubleClicked(const QModelIndex&)
{
  m_pDocument->TriggerFocusOnSelection(true);
}

void ezScenegraphPanel::OnRequestContextMenu(QPoint pos)
{
  ezMenuActionMapView menu(nullptr);

  ezActionContext context;
  context.m_sMapping = "ScenegraphContextMenu";
  context.m_pDocument = m_pDocument;
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));

}

void ezScenegraphPanel::OnFilterTextChanged(const QString& text)
{
  if (text.isEmpty())
  {
    m_pTreeWidget->GetProxyFilterModel()->setFilterWildcard(QString());
  }
  else
  {
    const QString pattern = QString("*%1*").arg(text);

    m_pTreeWidget->GetProxyFilterModel()->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_pTreeWidget->GetProxyFilterModel()->setFilterWildcard(pattern);
  }
}




