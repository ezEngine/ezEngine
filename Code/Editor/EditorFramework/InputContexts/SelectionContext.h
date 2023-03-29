#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

class QWidget;
class ezCamera;
struct ezObjectPickingResult;
class ezDocumentObject;

class EZ_EDITORFRAMEWORK_DLL ezSelectionContext : public ezEditorInputContext
{
public:
  ezSelectionContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera);
  ~ezSelectionContext();

  void SetWindowConfig(const ezVec2I32& vViewport) { m_vViewport = vViewport; }

  /// \brief Adds a delegate that gets called whenever an object is picked, as long as the override is active.
  ///
  /// It also changes the owner view's cursor to a cross-hair.
  /// If something gets picked, the override is called with a non-null object.
  /// In case the user presses ESC or the view gets destroyed while the override is active,
  /// the delegate is called with nullptr.
  /// This indicates that all picking should be stopped and the registered user should clean up.
  void SetPickObjectOverride(ezDelegate<void(const ezDocumentObject*)> pickOverride);
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

  virtual void OpenDocumentForPickedObject(const ezObjectPickingResult& res) const;
  virtual void SelectPickedObject(const ezObjectPickingResult& res, bool bToggle, bool bDirect) const;

protected:
  void SendMarqueeMsg(QMouseEvent* e, ezUInt8 uiWhatToDo);

  ezDelegate<void(const ezDocumentObject*)> m_PickObjectOverride;
  const ezCamera* m_pCamera;
  ezVec2I32 m_vViewport;
  ezEngineGizmoHandle m_hMarqueeGizmo;
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
