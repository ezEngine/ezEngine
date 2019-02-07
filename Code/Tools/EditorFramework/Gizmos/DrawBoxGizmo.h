#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class EZ_EDITORFRAMEWORK_DLL ezDrawBoxGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDrawBoxGizmo, ezGizmo);

public:
  enum class ManipulateMode
  {
    None,
    DrawBase,
    DrawHeight,
  };

  ezDrawBoxGizmo();
  ~ezDrawBoxGizmo();

  void GetResult(ezVec3& out_Origin, float& out_fSizeNegX, float& out_fSizePosX, float& out_fSizeNegY, float& out_fSizePosY, float& out_fSizeNegZ, float& out_fSizePosZ) const;

  ManipulateMode GetCurrentMode() const { return m_ManipulateMode; }
  const ezVec3& GetStartPosition() const { return m_vFirstCorner; }

  virtual void UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual ezEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;

private:

  void SwitchMode(bool bCancel);
  void UpdateBox();

  ManipulateMode m_ManipulateMode;
  ezEngineGizmoHandle m_Box;

  ezInt32 m_iHeightChange = 0;
  ezVec2I32 m_LastMousePos;
  ezVec3 m_vCurrentPosition;
  ezVec3 m_vFirstCorner;
  ezVec3 m_vSecondCorner;
  ezVec3 m_vUpAxis;
  ezVec3 m_vLastStartPoint;
  float m_fBoxHeight = 0.5f;
  float m_fOriginalBoxHeight = 0.5f;
};
