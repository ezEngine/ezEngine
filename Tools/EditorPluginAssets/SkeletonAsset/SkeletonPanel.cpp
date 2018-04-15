#include <PCH.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonPanel.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <QTreeView>
#include <QLayout>

ezQtSkeletonPanel::ezQtSkeletonPanel(QWidget* pParent, ezSkeletonAssetDocument* pDocument)
  : ezQtDocumentPanel(pParent)
{
  m_pSkeletonDocument = pDocument;

  setObjectName("SkeletonPanel");
  setWindowTitle("Skeleton");

  m_pMainWidget = new QWidget(this);
  m_pMainWidget->setLayout(new QVBoxLayout());
  m_pMainWidget->setContentsMargins(0, 0, 0, 0);
  m_pMainWidget->layout()->setContentsMargins(0, 0, 0, 0);
  m_pFilterWidget = new ezQtSearchWidget(this);
  connect(m_pFilterWidget, &ezQtSearchWidget::textChanged, this, [this](const QString& text)
  {
    m_pTreeWidget->GetProxyFilterModel()->SetFilterText(text);
  });

  m_pMainWidget->layout()->addWidget(m_pFilterWidget);

  std::unique_ptr<ezQtDocumentTreeModel> pModel(new ezQtDocumentTreeModel(pDocument->GetObjectManager()));
  pModel->AddAdapter(new ezQtDummyAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezDocumentRoot>(), "Children"));
  pModel->AddAdapter(new ezQtDummyAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezEditableSkeleton>(), "Children"));
  pModel->AddAdapter(new ezQtBoneAdapter(pDocument));

  m_pTreeWidget = new ezQtDocumentTreeView(this, pDocument, std::move(pModel));
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pMainWidget->layout()->addWidget(m_pTreeWidget);

  setWidget(m_pMainWidget);
}

ezQtSkeletonPanel::~ezQtSkeletonPanel() = default;

