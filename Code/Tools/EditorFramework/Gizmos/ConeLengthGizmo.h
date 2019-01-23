#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class EZ_EDITORFRAMEWORK_DLL ezConeLengthGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConeLengthGizmo, ezGizmo);

public:
  ezConeLengthGizmo();

  void SetRadius(float radius);
  float GetRadius() const { return m_fRadius; }

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

  ezEngineGizmoHandle m_ConeRadius;

  enum class ManipulateMode
  {
    None,
    Radius
  };

  ManipulateMode m_ManipulateMode;

  float m_fRadius;
  float m_fRadiusScale;
};
