#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonPanel.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>

ezQtSkeletonPanel::ezQtSkeletonPanel(ads::CDockManager* pDockManager, QWidget* pParent, ezSkeletonAssetDocument* pDocument)
  : ezQtDocumentPanel(pDockManager, pParent, pDocument)
{
  m_pSkeletonDocument = pDocument;

  setObjectName("SkeletonPanel");
  setWindowTitle("Skeleton");

  m_pMainWidget = new QWidget(this);
  m_pMainWidget->setLayout(new QVBoxLayout());
  m_pMainWidget->setContentsMargins(0, 0, 0, 0);
  m_pMainWidget->layout()->setContentsMargins(0, 0, 0, 0);
  m_pFilterWidget = new ezQtSearchWidget(this);
  connect(m_pFilterWidget, &ezQtSearchWidget::textChanged, this,
    [this](const QString& sText)
    { m_pTreeWidget->GetProxyFilterModel()->SetFilterText(sText); });

  m_pMainWidget->layout()->addWidget(m_pFilterWidget);

  std::unique_ptr<ezQtDocumentTreeModel> pModel(new ezQtDocumentTreeModel(pDocument->GetObjectManager()));
  pModel->AddAdapter(new ezQtDummyAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezDocumentRoot>(), "Children"));
  pModel->AddAdapter(new ezQtDummyAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezEditableSkeleton>(), "Children"));
  pModel->AddAdapter(new ezQtJointAdapter(pDocument));

  m_pTreeWidget = new ezQtDocumentTreeView(this, pDocument, std::move(pModel));
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(false);
  m_pTreeWidget->expandAll();
  m_pMainWidget->layout()->addWidget(m_pTreeWidget);

  setWidget(m_pMainWidget);
}

ezQtSkeletonPanel::~ezQtSkeletonPanel() = default;
