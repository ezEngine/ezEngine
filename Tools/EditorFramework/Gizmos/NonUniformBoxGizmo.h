#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class EZ_EDITORFRAMEWORK_DLL ezNonUniformBoxGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezNonUniformBoxGizmo, ezGizmo);

public:
  ezNonUniformBoxGizmo();

  void SetSize(const ezVec3& negSize, const ezVec3& posSize);

  const ezVec3& GetNegSize() const { return m_vNegSize; }
  const ezVec3& GetPosSize() const { return m_vPosSize; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;

private:
  ezResult GetPointOnAxis(ezInt32 iScreenPosX, ezInt32 iScreenPosY, ezVec3& out_Result) const;

  ezTime m_LastInteraction;
  ezMat4 m_InvViewProj;

  ezVec2I32 m_LastMousePos;

  ezEngineGizmoHandle m_Outline;
  ezEngineGizmoHandle m_Nobs[6];
  ezVec3 m_vMainAxis[6];

  enum ManipulateMode
  {
    None = -1,
    DragNegX,
    DragPosX,
    DragNegY,
    DragPosY,
    DragNegZ,
    DragPosZ,
  };

  ManipulateMode m_ManipulateMode = ManipulateMode::None;

  ezVec3 m_vNegSize;
  ezVec3 m_vPosSize;
  ezVec3 m_vStartNegSize;
  ezVec3 m_vStartPosSize;
  ezVec3 m_vMoveAxis;
  ezVec3 m_vStartPosition;
  ezVec3 m_vInteractionPivot;
  float m_fStartScale = 1.0f;
};
