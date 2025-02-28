#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerPanel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <QVBoxLayout>

ezQtLayerPanel::ezQtLayerPanel(ads::CDockManager* pDockManager, QWidget* pParent, ezScene2Document* pDocument)
  : ezQtDocumentPanel(pDockManager, pParent, pDocument)
{
  setObjectName("LayerPanel");
  setWindowTitle("Layers");
  m_pSceneDocument = pDocument;
  m_pDelegate = new ezQtLayerDelegate(this, pDocument);

  std::unique_ptr<ezQtLayerModel> pModel(new ezQtLayerModel(m_pSceneDocument));
  pModel->AddAdapter(new ezQtDummyAdapter(pDocument->GetSceneObjectManager(), ezGetStaticRTTI<ezSceneDocumentSettings>(), "Layers"));
  pModel->AddAdapter(new ezQtLayerAdapter(pDocument));

  m_pTreeWidget = new ezQtDocumentTreeView(this, pDocument, std::move(pModel), m_pSceneDocument->GetLayerSelectionManager());
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(false);
  m_pTreeWidget->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  m_pTreeWidget->setItemDelegate(m_pDelegate);

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  EZ_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr,
    "signal/slot connection failed");

  setWidget(m_pTreeWidget);
}

ezQtLayerPanel::~ezQtLayerPanel() = default;

void ezQtLayerPanel::OnRequestContextMenu(QPoint pos)
{
  ezQtMenuActionMapView menu(nullptr);

  ezActionContext context;
  context.m_sMapping = "EditorPluginScene_LayerContextMenu";
  context.m_pDocument = m_pSceneDocument;
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));
}
