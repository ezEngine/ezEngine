#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <Foundation/Math/Mat4.h>
#include <Core/World/GameObject.h>

class ezWorld;
class ezMeshComponent;
class ezGizmoBase;

enum ezGizmoHandleType
{
  Arrow,
  Ring,
  Rect,
  Box,
  Piston,
  HalfPiston,
};

class EZ_EDITORFRAMEWORK_DLL ezGizmoHandleBase : public ezEditorEngineSyncObject
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoHandleBase);

public:
  ezGizmoHandleBase(){}

  ezGizmoBase* GetParentGizmo() const { return m_pParentGizmo; }

  void SetVisible(bool bVisible);

  void SetTransformation(const ezMat4& m);

  const ezMat4& GetTransformation() const { return m_Transformation; }

protected:
  bool m_bVisible;
  ezMat4 m_Transformation;

  void SetParentGizmo(ezGizmoBase* pParentGizmo) { m_pParentGizmo = pParentGizmo; }

private:
  ezGizmoBase* m_pParentGizmo;
};

class EZ_EDITORFRAMEWORK_DLL ezGizmoHandle : public ezGizmoHandleBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoHandle);

public:
  ezGizmoHandle();

  void Configure(ezGizmoBase* pParentGizmo, ezGizmoHandleType type, const ezColor& col);

  bool IsSetupForEngine() const { return !m_hGameObject.IsInvalidated(); }

  bool SetupForEngine(ezWorld* pWorld, ezUInt32 uiNextComponentPickingID);
  void UpdateForEngine(ezWorld* pWorld);

protected:
  ezInt32 m_iHandleType;
  ezGameObjectHandle m_hGameObject;
  ezMeshComponent* m_pMeshComponent;
  ezColor m_Color;

private:

};

