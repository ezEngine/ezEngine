#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>

class ezQtSceneViewWidget : public ezQtGameObjectViewWidget
{
  Q_OBJECT
public:
  ezQtSceneViewWidget(QWidget* pParent, ezQtGameObjectDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtSceneViewWidget();

  virtual bool IsPickingAgainstSelectionAllowed() const override;

protected:
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* e) override;
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;
  virtual void OnOpenContextMenu(QPoint globalPos) override;

  bool m_bAllowPickSelectedWhileDragging;
  ezTime m_LastDragMoveEvent;

  static bool s_bContextMenuInitialized;
};
