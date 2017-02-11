#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class EZ_EDITORFRAMEWORK_DLL ezConeGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConeGizmo, ezGizmo);

public:
  ezConeGizmo();

  void SetAngle(ezAngle angle);
  void SetRadius(float radius);

  ezAngle GetAngle() const { return m_Angle; }
  float GetRadius() const { return m_fRadius; }

  void SetEnableRadiusHandle(bool enable) { m_bEnableRadiusHandle = enable; }

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
  ezEngineGizmoHandle m_ConeRadius;

  enum class ManipulateMode
  {
    None,
    Angle,
    Radius
  };

  ManipulateMode m_ManipulateMode;

  bool m_bEnableRadiusHandle;
  ezAngle m_Angle;
  float m_fRadius;
  float m_fAngleScale;
  float m_fRadiusScale;
};
