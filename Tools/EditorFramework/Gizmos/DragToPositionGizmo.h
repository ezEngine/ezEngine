#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Math/Quat.h>

class EZ_EDITORFRAMEWORK_DLL ezDragToPositionGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDragToPositionGizmo, ezGizmo);

public:
  ezDragToPositionGizmo();

  const ezVec3 GetTranslationResult() const { return GetTransformation().m_vPosition - m_vStartPosition; }
  const ezQuat GetRotationResult() const { return GetTransformation().m_qRotation; }

  virtual bool IsPickingSelectedAllowed() const { return false; }

  /// \brief Returns true if any of the 'align with' handles is selected, and thus the rotation of the dragged object should be modified as well
  bool ModifiesRotation() const { return m_bModifiesRotation; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInut DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseMoveEvent(QMouseEvent* e) override;
  
  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;

  ezEngineGizmoHandle m_Bobble;
  ezEngineGizmoHandle m_AlignPX;
  ezEngineGizmoHandle m_AlignNX;
  ezEngineGizmoHandle m_AlignPY;
  ezEngineGizmoHandle m_AlignNY;
  ezEngineGizmoHandle m_AlignPZ;
  ezEngineGizmoHandle m_AlignNZ;

  bool m_bModifiesRotation;
  ezTime m_LastInteraction;
  ezVec3 m_vStartPosition;
};


