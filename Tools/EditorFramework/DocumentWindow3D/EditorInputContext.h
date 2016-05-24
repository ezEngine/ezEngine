#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Math/Rect.h>

class QWidget;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class ezDocument;
class ezQtEngineDocumentWindow;
class ezQtEngineViewWidget;

enum class ezEditorInut
{
  MayBeHandledByOthers,
  WasExclusivelyHandled,
};

class EZ_EDITORFRAMEWORK_DLL ezEditorInputContext : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorInputContext, ezReflectedClass);

public:
  ezEditorInputContext();

  virtual ~ezEditorInputContext();

  virtual void FocusLost(bool bCancel);

  ezEditorInut keyPressEvent(QKeyEvent* e) { return doKeyPressEvent(e); }
  ezEditorInut keyReleaseEvent(QKeyEvent* e) { return doKeyReleaseEvent(e); }
  ezEditorInut mousePressEvent(QMouseEvent* e) { return doMousePressEvent(e); }
  ezEditorInut mouseReleaseEvent(QMouseEvent* e) { return doMouseReleaseEvent(e); }
  ezEditorInut mouseMoveEvent(QMouseEvent* e);
  ezEditorInut wheelEvent(QWheelEvent* e) { return doWheelEvent(e); }

  static void SetActiveInputContext(ezEditorInputContext* pContext) { s_pActiveInputContext = pContext; }

  void MakeActiveInputContext(bool bActive = true);

  static bool IsAnyInputContextActive() { return s_pActiveInputContext != nullptr; }

  static ezEditorInputContext* GetActiveInputContext() { return s_pActiveInputContext; }

  static void UpdateActiveInputContext();

  bool IsActiveInputContext() const;

  void SetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView);

  ezQtEngineDocumentWindow* GetOwnerWindow() const;

  ezQtEngineViewWidget* GetOwnerView() const;

  bool GetShortcutsDisabled() const { return m_bDisableShortcuts; }

  /// \brief If set to true, the surrounding window will ensure to block all shortcuts and instead send keypress events to the input context
  void SetShortcutsDisabled(bool bDisabled) { m_bDisableShortcuts = bDisabled; }

  virtual bool IsPickingSelectedAllowed() const { return true; }

  enum class MouseMode
  {
    Normal,
    WrapAtScreenBorders,
    HideAndWrapAtScreenBorders,
  };

  void SetMouseMode(MouseMode mode);

  ezVec2I32 UpdateMouseMode(QMouseEvent* e);

protected:
  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) = 0;

  virtual ezEditorInut doKeyPressEvent(QKeyEvent* e);
  virtual ezEditorInut doKeyReleaseEvent(QKeyEvent* e) { return ezEditorInut::MayBeHandledByOthers; }
  virtual ezEditorInut doMousePressEvent(QMouseEvent* e) { return ezEditorInut::MayBeHandledByOthers; }
  virtual ezEditorInut doMouseReleaseEvent(QMouseEvent* e) { return ezEditorInut::MayBeHandledByOthers; }
  virtual ezEditorInut doMouseMoveEvent(QMouseEvent* e) { return ezEditorInut::MayBeHandledByOthers; }
  virtual ezEditorInut doWheelEvent(QWheelEvent* e) { return ezEditorInut::MayBeHandledByOthers; }

private:
  static ezEditorInputContext* s_pActiveInputContext;

  ezQtEngineDocumentWindow* m_pOwnerWindow;
  ezQtEngineViewWidget* m_pOwnerView;
  bool m_bDisableShortcuts;
  bool m_bJustWrappedMouse;
  MouseMode m_MouseMode;
  ezVec2I32 m_MouseRestorePosition;
  ezVec2I32 m_MousePosBeforeWrap;
  ezVec2I32 m_ExpectedMousePosition;
  ezRectU32 m_MouseWrapRect;

  virtual void UpdateContext() {}
};






