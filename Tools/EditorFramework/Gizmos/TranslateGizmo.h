#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>

class EZ_EDITORFRAMEWORK_DLL ezTranslateGizmo
{
public:
  ezTranslateGizmo();

  void SetDocumentGuid(const ezUuid& guid);

  void SetVisible(bool bVisible);
  void SetTransformation(const ezMat4& transform);

private:
  bool m_bVisible;
  ezMat4 m_Transformation;

  ezEditorGizmoHandle m_AxisX;
  ezEditorGizmoHandle m_AxisY;
  ezEditorGizmoHandle m_AxisZ;

};
