#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>


ezQtGameObjectWidget::ezQtGameObjectWidget(QWidget* pParent, ezGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel, ezSelectionManager* pSelection)
{
  setObjectName("ezQtGameObjectWidget");

  m_pDocument = pDocument;
  m_sContextMenuMapping = szContextMenuMapping;
  m_pDelegate = new ezQtGameObjectDelegate(this, pDocument);

  setLayout(new QVBoxLayout());
  setContentsMargins(0, 0, 0, 0);
  layout()->setObjectName("QVBoxLayout1");
  layout()->setContentsMargins(0, 0, 0, 0);

  m_pFilterWidget = new ezQtSearchWidget(this);
  m_pFilterWidget->setObjectName("ezQtSearchWidget");
  connect(m_pFilterWidget, &ezQtSearchWidget::textChanged, this, &ezQtGameObjectWidget::OnFilterTextChanged);

  layout()->addWidget(m_pFilterWidget);

  m_pTreeWidget = new ezQtDocumentTreeView(this, pDocument, std::move(pCustomModel), pSelection);
  m_pTreeWidget->setObjectName("ezQtDocumentTreeView");
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(true);
  layout()->addWidget(m_pTreeWidget);
  m_pTreeWidget->setItemDelegate(m_pDelegate);

  m_pDocument->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezQtGameObjectWidget::DocumentSceneEventHandler, this));

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(m_pTreeWidget, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnItemDoubleClicked(const QModelIndex&))) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr, "signal/slot connection failed");
}

ezQtGameObjectWidget::~ezQtGameObjectWidget()
{
  m_pDocument->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezQtGameObjectWidget::DocumentSceneEventHandler, this));
}


void ezQtGameObjectWidget::DocumentSceneEventHandler(const ezGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezGameObjectEvent::Type::TriggerShowSelectionInScenegraph:
    {
      m_pTreeWidget->EnsureLastSelectedItemVisible();
    }
    break;

    default:
      break;
  }
}

void ezQtGameObjectWidget::OnItemDoubleClicked(const QModelIndex&)
{
  m_pDocument->TriggerFocusOnSelection(true);
}

void ezQtGameObjectWidget::OnRequestContextMenu(QPoint pos)
{
  ezQtMenuActionMapView menu(nullptr);

  ezActionContext context;
  context.m_sMapping = m_sContextMenuMapping;
  context.m_pDocument = m_pDocument;
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));
}

void ezQtGameObjectWidget::OnFilterTextChanged(const QString& text)
{
  m_pTreeWidget->GetProxyFilterModel()->SetFilterText(text);
}


//////////////////////////////////////////////////////////////////////////

ezQtGameObjectPanel::ezQtGameObjectPanel(ads::CDockManager* pDockManager, QWidget* pParent, ezGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel)
  : ezQtDocumentPanel(pDockManager, pParent, pDocument)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("ezQtGameObjectPanel");

  m_pMainWidget = new ezQtGameObjectWidget(this, pDocument, szContextMenuMapping, std::move(pCustomModel));
  setWidget(m_pMainWidget);
}

ezQtGameObjectPanel::~ezQtGameObjectPanel() = default;
