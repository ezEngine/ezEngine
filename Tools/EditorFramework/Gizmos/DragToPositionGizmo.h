#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class EZ_EDITORFRAMEWORK_DLL ezDragToPositionGizmo : public ezGizmoBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDragToPositionGizmo);

public:
  ezDragToPositionGizmo();

  virtual void SetDocumentGuid(const ezUuid& guid) override;

  virtual void FocusLost() override;

  virtual bool mousePressEvent(QMouseEvent* e) override;
  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;

  const ezVec3 GetTranslationResult() const { return GetTransformation().GetTranslationVector() - m_vStartPosition; }

protected:
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;

  ezGizmoHandle m_Bobble;

  ezTime m_LastInteraction;
  ezVec3 m_vStartPosition;
};


