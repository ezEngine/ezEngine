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
  ezEditorGizmoHandle() { m_bVisible = false; m_Transformation.SetIdentity(); m_Color = ezColor::CornflowerBlue; /* The Original! */ }
  virtual ~ezEditorGizmoHandle() { }

  void SetVisible(bool bVisible) { m_bVisible = bVisible; SetModified(true); }

  void SetTransformation(const ezMat4& m) { m_Transformation = m; SetModified(true); }

  const ezMat4& GetTransformation() const { return m_Transformation; }

  bool IsSetupForEngine() const { return !m_hGameObject.IsInvalidated(); }

  bool SetupForEngine(ezWorld* pWorld, ezUInt32 uiNextComponentPickingID);
  void UpdateForEngine(ezWorld* pWorld);

  void SetColor(const ezColor& col) { m_Color = col; }

protected:
  bool m_bVisible;
  ezMat4 m_Transformation;
  ezInt32 m_iHandleType;
  ezGameObjectHandle m_hGameObject;
  ezColor m_Color;

private:

};

