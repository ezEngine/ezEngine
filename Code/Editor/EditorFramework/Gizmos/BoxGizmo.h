#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class EZ_EDITORFRAMEWORK_DLL ezBoxGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBoxGizmo, ezGizmo);

public:
  ezBoxGizmo();

  void SetSize(const ezVec3& size);

  const ezVec3& GetSize() const { return m_vSize; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;


private:
  ezTime m_LastInteraction;

  ezVec2I32 m_LastMousePos;

  ezEngineGizmoHandle m_Corners;
  ezEngineGizmoHandle m_Edges[3];
  ezEngineGizmoHandle m_Faces[3];

  enum class ManipulateMode
  {
    None,
    Uniform,
    AxisX,
    AxisY,
    AxisZ,
    PlaneXY,
    PlaneXZ,
    PlaneYZ,
  };

  ManipulateMode m_ManipulateMode;

  ezVec3 m_vSize;
};
