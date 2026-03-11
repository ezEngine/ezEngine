#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <Foundation/Basics.h>

class ezScene2Document;
class ezQtLayerDelegate;

class ezQtLayerPanel : public ezQtDocumentPanel
{
  Q_OBJECT

public:
  ezQtLayerPanel(ads::CDockManager* pDockManager, QWidget* pParent, ezScene2Document* pDocument);
  ~ezQtLayerPanel();

private Q_SLOTS:
  void OnRequestContextMenu(QPoint pos);

private:
  ezQtLayerDelegate* m_pDelegate = nullptr;
  ezScene2Document* m_pSceneDocument = nullptr;
  ezQtDocumentTreeView* m_pTreeWidget = nullptr;
  ezString m_sContextMenuMapping;
};
