#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

class ezViewMarqueePickingResultMsgToEditor;
class ezQtGameObjectDocumentWindow;
class ezOrthoGizmoContext;
class ezContextMenuContext;
class ezSelectionContext;
class ezCameraMoveContext;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtGameObjectViewWidget(QWidget* pParent, ezQtGameObjectDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtGameObjectViewWidget();

  ezOrthoGizmoContext* m_pOrthoGizmoContext;
  ezSelectionContext* m_pSelectionContext;
  ezCameraMoveContext* m_pCameraMoveContext;

  virtual void SyncToEngine() override;

protected:
  virtual void HandleMarqueePickingResult(const ezViewMarqueePickingResultMsgToEditor* pMsg) override;

  ezUInt32 m_uiLastMarqueeActionID = 0;
  ezDeque<ezUuid> m_MarqueeBaseSelection;
};


