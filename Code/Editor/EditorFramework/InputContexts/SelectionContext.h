#pragma once

#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>

class QWidget;
class ezCamera;
struct ezObjectPickingResult;
class ezDocumentObject;

class EZ_EDITORFRAMEWORK_DLL ezSelectionContext : public ezEditorInputContext
{
public:
  ezSelectionContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera);

  void SetWindowConfig(const ezVec2I32& viewport)
  {
    m_Viewport = viewport;
  }

  void SetPickObjectOverride(ezDelegate<bool(const ezDocumentObject*)> pickOverride);
  void ResetPickObjectOverride();

protected:
  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;

  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual ezEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}

  const ezDocumentObject* determineObjectToSelect(const ezDocumentObject* pickedObject, bool bToggle, bool bDirect) const;

  virtual void DoFocusLost(bool bCancel) override;

private:
  void OpenDocumentForPickedObject(const ezObjectPickingResult& res) const;
  void SendMarqueeMsg(QMouseEvent* e, ezUInt8 uiWhatToDo);

  ezDelegate<bool(const ezDocumentObject*)> m_PickObjectOverride;
  const ezCamera* m_pCamera;
  ezVec2I32 m_Viewport;
  ezEngineGizmoHandle m_MarqueeGizmo;
  ezVec3 m_vMarqueeStartPos;
  ezUInt32 m_uiMarqueeID;
  bool m_bPressedSpace = false;

  enum class Mode
  {
    None,
    Single,
    MarqueeAdd,
    MarqueeRemove
  };

  Mode m_Mode = Mode::None;
};

