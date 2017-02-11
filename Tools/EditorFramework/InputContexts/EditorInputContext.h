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

  void FocusLost(bool bCancel);

  ezEditorInut KeyPressEvent(QKeyEvent* e) { return DoKeyPressEvent(e); }
  ezEditorInut KeyReleaseEvent(QKeyEvent* e) { return DoKeyReleaseEvent(e); }
  ezEditorInut MousePressEvent(QMouseEvent* e) { return DoMousePressEvent(e); }
  ezEditorInut MouseReleaseEvent(QMouseEvent* e) { return DoMouseReleaseEvent(e); }
  ezEditorInut MouseMoveEvent(QMouseEvent* e);
  ezEditorInut WheelEvent(QWheelEvent* e) { return DoWheelEvent(e); }

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

  /// \brief How the mouse position is updated when the mouse cursor reaches the screen borders.
  enum class MouseMode
  {
    Normal, ///< Nothing happens, the mouse will stop at screen borders as usual
    WrapAtScreenBorders, ///< The mouse is visibly wrapped at screen borders. When this mode is disabled, the mouse stays where it is.
    HideAndWrapAtScreenBorders, ///< The mouse is wrapped at screen borders, which enables infinite movement, but the cursor is invisible. When this mode is disabled the mouse is restored to the position where it was when it was enabled.
  };

  /// \brief Sets how the mouse will act when it reaches the screen border. UpdateMouseMode() must be called on every mouseMoveEvent to update the state.
  ///
  /// The return value is the current global mouse position. Can be used to initialize a 'Last Mouse Position' variable.
  ezVec2I32 SetMouseMode(MouseMode mode);

  /// \brief Updates the mouse position. Can always be called but will only have an effect if SetMouseMode() was called with one of the wrap modes.
  ///
  /// Returns the new global mouse position, which may change drastically if the mouse cursor needed to be wrapped around the screen.
  /// Should be used to update a "Last Mouse Position" variable.
  ezVec2I32 UpdateMouseMode(QMouseEvent* e);

protected:
  virtual void DoFocusLost(bool bCancel) {}

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) = 0;

  virtual ezEditorInut DoKeyPressEvent(QKeyEvent* e);
  virtual ezEditorInut DoKeyReleaseEvent(QKeyEvent* e) { return ezEditorInut::MayBeHandledByOthers; }
  virtual ezEditorInut DoMousePressEvent(QMouseEvent* e) { return ezEditorInut::MayBeHandledByOthers; }
  virtual ezEditorInut DoMouseReleaseEvent(QMouseEvent* e) { return ezEditorInut::MayBeHandledByOthers; }
  virtual ezEditorInut DoMouseMoveEvent(QMouseEvent* e) { return ezEditorInut::MayBeHandledByOthers; }
  virtual ezEditorInut DoWheelEvent(QWheelEvent* e) { return ezEditorInut::MayBeHandledByOthers; }

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






