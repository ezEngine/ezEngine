#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

class ezSkeletonAssetDocument;
class QTreeView;
class ezQtSkeletonModel;

class ezQtSkeletonPanel : public ezQtDocumentPanel
{
  Q_OBJECT

public:
  ezQtSkeletonPanel(QWidget* pParent, ezSkeletonAssetDocument* pDocument);
  ~ezQtSkeletonPanel();

private:
  ezSkeletonAssetDocument* m_pSkeletonDocument = nullptr;
  QWidget* m_pMainWidget = nullptr;
  QTreeView* m_pTreeWidget = nullptr;
  ezQtSkeletonModel* m_pSkeletonModel = nullptr;
};
