#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>

class QWidget;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class ezDocumentBase;
class ezDocumentWindow3D;
class ezEngineViewWidget;

class EZ_EDITORFRAMEWORK_DLL ezEditorInputContext : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorInputContext);

public:
  ezEditorInputContext()
  {
    m_pOwnerWindow = nullptr;
    m_pOwnerView = nullptr;
    m_bDisableShortcuts = false;
  }

  virtual ~ezEditorInputContext();

  virtual void FocusLost() {}

  virtual bool keyPressEvent(QKeyEvent* e) { return false; }
  virtual bool keyReleaseEvent(QKeyEvent* e) { return false; }
  virtual bool mousePressEvent(QMouseEvent* e) { return false; }
  virtual bool mouseReleaseEvent(QMouseEvent* e) { return false; }
  virtual bool mouseMoveEvent(QMouseEvent* e) { return false; }
  virtual bool wheelEvent(QWheelEvent* e) { return false; }

  static void SetActiveInputContext(ezEditorInputContext* pContext) { s_pActiveInputContext = pContext; }

  void MakeActiveInputContext(bool bActive = true)
  {
    if (bActive)
      s_pActiveInputContext = this;
    else
      s_pActiveInputContext = nullptr;
  }

  static bool IsAnyInputContextActive() { return s_pActiveInputContext != nullptr; }

  static ezEditorInputContext* GetActiveInputContext() { return s_pActiveInputContext; }

  static void UpdateActiveInputContext()
  {
    if (s_pActiveInputContext != nullptr)
      s_pActiveInputContext->UpdateContext();
  }

  bool IsActiveInputContext() const
  {
    return s_pActiveInputContext == this;
  }

  void SetOwner(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView)
  {
    m_pOwnerWindow = pOwnerWindow;
    m_pOwnerView = pOwnerView;

    OnSetOwner(m_pOwnerWindow, m_pOwnerView);
  }

  ezDocumentWindow3D* GetOwnerWindow() const
  {
    EZ_ASSERT_DEBUG(m_pOwnerWindow != nullptr, "Owner window pointer has not been set");
    return m_pOwnerWindow;
  }

  ezEngineViewWidget* GetOwnerView() const
  {
    EZ_ASSERT_DEBUG(m_pOwnerView != nullptr, "Owner view pointer has not been set");
    return m_pOwnerView;
  }

  bool GetShortcutsDisabled() const { return m_bDisableShortcuts; }

  /// \brief If set to true, the surrounding window will ensure to block all shortcuts and instead send keypress events to the input context
  void SetShortcutsDisabled(bool bDisabled) { m_bDisableShortcuts = bDisabled; }

protected:
  virtual void OnSetOwner(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView) = 0;

private:
  static ezEditorInputContext* s_pActiveInputContext;

  ezDocumentWindow3D* m_pOwnerWindow;
  ezEngineViewWidget* m_pOwnerView;
  bool m_bDisableShortcuts;

  virtual void UpdateContext() {}
};