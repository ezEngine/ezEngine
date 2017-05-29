#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <Foundation/Math/Mat4.h>
#include <Core/World/GameObject.h>

class ezWorld;
class ezGizmoComponent;
class ezGizmo;

class EZ_EDITORFRAMEWORK_DLL ezGizmoHandle : public ezEditorEngineSyncObject
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoHandle, ezEditorEngineSyncObject);

public:
  ezGizmoHandle();

  ezGizmo* GetOwnerGizmo() const { return m_pParentGizmo; }

  void SetVisible(bool bVisible);

  void SetTransformation(const ezTransform& m);

  const ezTransform& GetTransformation() const { return m_Transformation; }

protected:
  bool m_bVisible;
  ezTransform m_Transformation;

  void SetParentGizmo(ezGizmo* pParentGizmo) { m_pParentGizmo = pParentGizmo; }

private:
  ezGizmo* m_pParentGizmo;
};


enum ezEngineGizmoHandleType
{
  Arrow,
  Ring,
  Rect,
  Box,
  Piston,
  HalfPiston,
  Sphere,
  CylinderZ,
  HalfSphereZ,
  BoxCorners,
  BoxEdges,
  BoxFaces,
  LineBox,
  Cone,
};

class EZ_EDITORFRAMEWORK_DLL ezEngineGizmoHandle : public ezGizmoHandle
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineGizmoHandle, ezGizmoHandle);

public:
  ezEngineGizmoHandle();
  ~ezEngineGizmoHandle();

  void Configure(ezGizmo* pParentGizmo, ezEngineGizmoHandleType type, const ezColor& col, bool bConstantSize = true, bool bAlwaysOnTop = false, bool bVisualizer = false);

  virtual bool SetupForEngine(ezWorld* pWorld, ezUInt32 uiNextComponentPickingID) override;
  virtual void UpdateForEngine(ezWorld* pWorld) override;

  void SetColor(const ezColor& col);

protected:
  bool m_bConstantSize;
  bool m_bAlwaysOnTop;
  bool m_bVisualizer;
  ezInt32 m_iHandleType;
  ezGameObjectHandle m_hGameObject;
  ezGizmoComponent* m_pGizmoComponent;
  ezColor m_Color;
  ezWorld* m_pWorld;

private:

};

