#include <PCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QBoxLayout>
#include <QMenu>
#include <QSortFilterProxyModel>

ezQtGameObjectPanel::ezQtGameObjectPanel(QWidget* pParent, ezGameObjectDocument* pDocument, const char* szContextMenuMapping,
                                         std::unique_ptr<ezQtDocumentTreeModel> pCustomModel)
    : ezQtDocumentPanel(pParent)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("Scenegraph");

  m_pDocument = pDocument;
  m_sContextMenuMapping = szContextMenuMapping;

  m_pMainWidget = new QWidget(this);
  m_pMainWidget->setLayout(new QVBoxLayout());
  m_pMainWidget->setContentsMargins(0, 0, 0, 0);
  m_pMainWidget->layout()->setContentsMargins(0, 0, 0, 0);

  m_pFilterWidget = new ezQtSearchWidget(this);
  connect(m_pFilterWidget, &ezQtSearchWidget::textChanged, this, &ezQtGameObjectPanel::OnFilterTextChanged);

  m_pMainWidget->layout()->addWidget(m_pFilterWidget);

  m_pTreeWidget = new ezQtDocumentTreeView(this, pDocument, std::move(pCustomModel));
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(true);
  m_pMainWidget->layout()->addWidget(m_pTreeWidget);

  setWidget(m_pMainWidget);

  m_pDocument->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezQtGameObjectPanel::DocumentSceneEventHandler, this));

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(m_pTreeWidget, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnItemDoubleClicked(const QModelIndex&))) !=
                nullptr,
            "signal/slot connection failed");
  EZ_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr,
            "signal/slot connection failed");
}

ezQtGameObjectPanel::~ezQtGameObjectPanel()
{
  m_pDocument->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezQtGameObjectPanel::DocumentSceneEventHandler, this));
}

void ezQtGameObjectPanel::DocumentSceneEventHandler(const ezGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezGameObjectEvent::Type::TriggerShowSelectionInScenegraph:
    {
      m_pTreeWidget->EnsureLastSelectedItemVisible();
    }
    break;
  }
}

void ezQtGameObjectPanel::OnItemDoubleClicked(const QModelIndex&)
{
  m_pDocument->TriggerFocusOnSelection(true);
}

void ezQtGameObjectPanel::OnRequestContextMenu(QPoint pos)
{
  ezQtMenuActionMapView menu(nullptr);

  ezActionContext context;
  context.m_sMapping = m_sContextMenuMapping;
  context.m_pDocument = m_pDocument;
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));
}

void ezQtGameObjectPanel::OnFilterTextChanged(const QString& text)
{
  m_pTreeWidget->GetProxyFilterModel()->SetFilterText(text);
}
