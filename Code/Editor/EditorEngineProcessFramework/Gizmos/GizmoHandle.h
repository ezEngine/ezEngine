#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Math/Mat4.h>
#include <Core/World/GameObject.h>

class ezWorld;
class ezGizmoComponent;
class ezGizmo;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGizmoHandle : public ezEditorEngineSyncObject
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoHandle, ezEditorEngineSyncObject);

public:
  ezGizmoHandle();

  ezGizmo* GetOwnerGizmo() const { return m_pParentGizmo; }

  void SetVisible(bool bVisible);

  void SetTransformation(const ezTransform& m);
  void SetTransformation(const ezMat4& m);

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
  LineRect,
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
  Frustum,
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEngineGizmoHandle : public ezGizmoHandle
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineGizmoHandle, ezGizmoHandle);

public:
  ezEngineGizmoHandle();
  ~ezEngineGizmoHandle();

  void Configure(ezGizmo* pParentGizmo, ezEngineGizmoHandleType type, const ezColor& col, bool bConstantSize = true, bool bAlwaysOnTop = false, bool bVisualizer = false, bool bShowInOrtho = false, bool bIsPickable = true);

  virtual bool SetupForEngine(ezWorld* pWorld, ezUInt32 uiNextComponentPickingID) override;
  virtual void UpdateForEngine(ezWorld* pWorld) override;

  void SetColor(const ezColor& col);

protected:
  bool m_bConstantSize;
  bool m_bAlwaysOnTop;
  bool m_bVisualizer;
  bool m_bShowInOrtho;
  bool m_bIsPickable = true;
  ezInt32 m_iHandleType;
  ezGameObjectHandle m_hGameObject;
  ezGizmoComponent* m_pGizmoComponent;
  ezColor m_Color;
  ezWorld* m_pWorld;
};

