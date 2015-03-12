#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <Foundation/Math/Mat4.h>

class EZ_EDITORFRAMEWORK_DLL ezEditorGizmoHandle : public ezEditorEngineSyncObject
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorGizmoHandle);

public:
  ezEditorGizmoHandle() { m_bVisible = false; m_Transformation.SetIdentity(); }
  virtual ~ezEditorGizmoHandle() { }

  void SetVisible(bool bVisible) { m_bVisible = bVisible; SetModified(true); }

  void SetTransformation(const ezMat4& m) { m_Transformation = m; SetModified(true); }

  const ezMat4& GetTransformation() const { return m_Transformation; }

protected:
  bool m_bVisible;
  ezMat4 m_Transformation;

private:

};

