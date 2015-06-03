#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <Foundation/Math/Mat4.h>
#include <Core/World/GameObject.h>

class ezWorld;
class ezMeshComponent;

enum class ezGizmoHandleType
{
  Arrow,
  Ring,
  Rect,
  Box,
  Piston,
};

class EZ_EDITORFRAMEWORK_DLL ezEditorGizmoHandle : public ezEditorEngineSyncObject
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorGizmoHandle);

public:
  ezEditorGizmoHandle();
  virtual ~ezEditorGizmoHandle() { }

  void Configure(ezGizmoHandleType type, const ezColor& col);

  void SetVisible(bool bVisible) { m_bVisible = bVisible; SetModified(true); }

  void SetTransformation(const ezMat4& m) { m_Transformation = m; SetModified(true); }

  const ezMat4& GetTransformation() const { return m_Transformation; }

  bool IsSetupForEngine() const { return !m_hGameObject.IsInvalidated(); }

  bool SetupForEngine(ezWorld* pWorld, ezUInt32 uiNextComponentPickingID);
  void UpdateForEngine(ezWorld* pWorld);

  

protected:
  bool m_bVisible;
  ezMat4 m_Transformation;
  ezInt32 m_iHandleType;
  ezGameObjectHandle m_hGameObject;
  ezMeshComponent* m_pMeshComponent;
  ezColor m_Color;

private:

};

