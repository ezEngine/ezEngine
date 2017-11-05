#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>

class ezQtPropertyAnimViewWidget : public ezQtGameObjectViewWidget
{
  Q_OBJECT
public:
  ezQtPropertyAnimViewWidget(QWidget* pParent, ezQtGameObjectDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtPropertyAnimViewWidget();
};
