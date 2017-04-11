#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class EZ_EDITORFRAMEWORK_DLL ezConeAngleGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConeAngleGizmo, ezGizmo);

public:
  ezConeAngleGizmo();

  void SetAngle(ezAngle angle);
  ezAngle GetAngle() const { return m_Angle; }

  void SetRadius(float radius) { m_fRadius = radius; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInut DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;


private:
  ezTime m_LastInteraction;

  ezVec2I32 m_LastMousePos;

  ezEngineGizmoHandle m_ConeAngle;

  enum class ManipulateMode
  {
    None,
    Angle,
  };

  ManipulateMode m_ManipulateMode;

  ezAngle m_Angle;
  float m_fRadius;
  float m_fAngleScale;
};
