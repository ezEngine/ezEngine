#include <PCH.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonPanel.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>
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

  m_pTreeWidget = new QTreeView(this);
  m_pMainWidget->layout()->addWidget(m_pTreeWidget);

  m_pSkeletonModel = new ezQtSkeletonModel(this, pDocument);
  m_pTreeWidget->setModel(m_pSkeletonModel);

  setWidget(m_pMainWidget);
}

ezQtSkeletonPanel::~ezQtSkeletonPanel() = default;

