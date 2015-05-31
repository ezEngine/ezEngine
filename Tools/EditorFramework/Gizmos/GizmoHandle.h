#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <Foundation/Math/Mat4.h>
#include <Core/World/GameObject.h>

class ezWorld;

class EZ_EDITORFRAMEWORK_DLL ezEditorGizmoHandle : public ezEditorEngineSyncObject
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorGizmoHandle);

public:
  ezEditorGizmoHandle() { m_bVisible = false; m_Transformation.SetIdentity(); }
  virtual ~ezEditorGizmoHandle() { }

  void SetVisible(bool bVisible) { m_bVisible = bVisible; SetModified(true); }

  void SetTransformation(const ezMat4& m) { m_Transformation = m; SetModified(true); }

  const ezMat4& GetTransformation() const { return m_Transformation; }

  bool IsSetupForEngine() const { return !m_hGameObject.IsInvalidated(); }

  void SetupForEngine(ezWorld* pWorld);
  void UpdateForEngine(ezWorld* pWorld);

protected:
  bool m_bVisible;
  ezMat4 m_Transformation;
  ezInt32 m_iHandleType;
  ezGameObjectHandle m_hGameObject;

private:

};

