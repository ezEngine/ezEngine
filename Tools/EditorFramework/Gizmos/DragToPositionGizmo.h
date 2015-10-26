#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class EZ_EDITORFRAMEWORK_DLL ezDragToPositionGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDragToPositionGizmo);

public:
  ezDragToPositionGizmo();

  virtual void FocusLost(bool bCancel) override;

  virtual bool mousePressEvent(QMouseEvent* e) override;
  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;

  const ezVec3 GetTranslationResult() const { return GetTransformation().GetTranslationVector() - m_vStartPosition; }
  const ezQuat GetRotationResult() const { ezQuat q; q.SetFromMat3(GetTransformation().GetRotationalPart()); return q; }

protected:
  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;

  ezEngineGizmoHandle m_Bobble;
  ezEngineGizmoHandle m_AlignPX;
  ezEngineGizmoHandle m_AlignNX;
  ezEngineGizmoHandle m_AlignPY;
  ezEngineGizmoHandle m_AlignNY;
  ezEngineGizmoHandle m_AlignPZ;
  ezEngineGizmoHandle m_AlignNZ;

  ezTime m_LastInteraction;
  ezVec3 m_vStartPosition;
};


