#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class EZ_EDITORFRAMEWORK_DLL ezSphereGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSphereGizmo, ezGizmo);

public:
  ezSphereGizmo();

  virtual void FocusLost(bool bCancel) override;

  void SetInnerSphere(bool bEnabled, float fRadius = 0.0f);
  void SetOuterSphere(float fRadius);

  float GetInnerRadius() const { return m_fRadiusInner; }
  float GetOuterRadius() const { return m_fRadiusOuter; }

protected:
  virtual ezEditorInut doMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut doMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut doMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;


private:
  ezTime m_LastInteraction;

  ezVec2 m_MousePos;

  ezEngineGizmoHandle m_InnerSphere;
  ezEngineGizmoHandle m_OuterSphere;

  enum class ManipulateMode
  {
    None,
    InnerSphere,
    OuterSphere
  };

  ManipulateMode m_ManipulateMode;
  bool m_bInnerEnabled;

  float m_fRadiusInner;
  float m_fRadiusOuter;
};
