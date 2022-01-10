#pragma once

#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Math/Mat4.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

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
  bool m_bVisible = false;
  ezTransform m_Transformation;

  void SetParentGizmo(ezGizmo* pParentGizmo) { m_pParentGizmo = pParentGizmo; }

private:
  ezGizmo* m_pParentGizmo = nullptr;
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
  FromFile,
};

struct ezGizmoFlags
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Default = 0,

    ConstantSize = EZ_BIT(0),
    OnTop = EZ_BIT(1),
    Visualizer = EZ_BIT(2),
    ShowInOrtho = EZ_BIT(3),
    Pickable = EZ_BIT(4),
  };

  struct Bits
  {
    StorageType ConstantSize : 1;
    StorageType OnTop : 1;
    StorageType Visualizer : 1;
    StorageType ShowInOrtho : 1;
    StorageType Pickable : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezGizmoFlags);

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEngineGizmoHandle : public ezGizmoHandle
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineGizmoHandle, ezGizmoHandle);

public:
  ezEngineGizmoHandle();
  ~ezEngineGizmoHandle();

  void ConfigureHandle(ezGizmo* pParentGizmo, ezEngineGizmoHandleType type, const ezColor& col, ezBitflags<ezGizmoFlags> flags, const char* szCustomMesh = nullptr);

  virtual bool SetupForEngine(ezWorld* pWorld, ezUInt32 uiNextComponentPickingID) override;
  virtual void UpdateForEngine(ezWorld* pWorld) override;

  void SetColor(const ezColor& col);

protected:
  bool m_bConstantSize = true;
  bool m_bAlwaysOnTop = false;
  bool m_bVisualizer = false;
  bool m_bShowInOrtho = false;
  bool m_bIsPickable = true;
  ezInt32 m_iHandleType = -1;
  ezString m_sGizmoHandleMesh;
  ezGameObjectHandle m_hGameObject;
  ezGizmoComponent* m_pGizmoComponent = nullptr;
  ezColor m_Color = ezColor::CornflowerBlue; /* The Original! */
  ezWorld* m_pWorld = nullptr;
};
