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

  virtual ezEditorInut mousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut mouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut mouseMoveEvent(QMouseEvent* e) override;

  void SetInnerSphere(float fRadius);
  void SetOuterSphere(bool bEnabled, float fRadius = 100000.0f);

  float GetInnerRadius() const { return m_fRadiusInner; }
  float GetOuterRadius() const { return m_fRadiusOuter; }

protected:
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
  bool m_bOuterEnabled;

  float m_fStartRadiusInner;
  float m_fStartRadiusOuter;

  float m_fRadiusInner;
  float m_fRadiusOuter;
};
