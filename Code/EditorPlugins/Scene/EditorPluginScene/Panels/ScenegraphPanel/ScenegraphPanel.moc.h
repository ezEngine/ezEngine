#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <Foundation/Basics.h>

class ezQtSearchWidget;
class ezQtDocumentTreeView;
class ezSceneDocument;

class ezQtScenegraphPanel : public ezQtGameObjectPanel
{
  Q_OBJECT

public:
  ezQtScenegraphPanel(QWidget* pParent, ezSceneDocument* pDocument);
  ~ezQtScenegraphPanel();

private:
  ezSceneDocument* m_pSceneDocument;
};

