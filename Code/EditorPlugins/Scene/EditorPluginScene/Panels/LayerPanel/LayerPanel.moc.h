#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <Foundation/Basics.h>

class ezScene2Document;

class ezQtLayerPanel : public ezQtDocumentPanel
{
  Q_OBJECT

public:
  ezQtLayerPanel(QWidget* pParent, ezScene2Document* pDocument);
  ~ezQtLayerPanel();

private Q_SLOTS:
  void OnRequestContextMenu(QPoint pos);

private:
  ezScene2Document* m_pSceneDocument;
  ezQtDocumentTreeView* m_pTreeWidget;
  ezString m_sContextMenuMapping;
};
