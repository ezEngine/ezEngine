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

protected:
  virtual ezEditorInut DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseReleaseEvent(QMouseEvent* e) override;

  virtual ezEditorInut DoMouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoKeyPressEvent(QKeyEvent* e) override;
  virtual ezEditorInut DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}

  const ezDocumentObject* determineObjectToSelect(const ezDocumentObject* pickedObject, bool bToggle, bool bDirect) const;

  virtual void DoFocusLost(bool bCancel) override;

private:
  void OpenPickedMaterial(const ezObjectPickingResult& res) const;
  bool TryOpenMaterial(const ezString& sMatRef) const;
  void SendMarqueeMsg(QMouseEvent* e, ezUInt8 uiWhatToDo);

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

