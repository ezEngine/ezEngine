#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Meshes/MeshComponent.h>

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGizmoRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoRenderData, ezMeshRenderData);

public:
  ezTransform m_GlobalTransform;
  ezColor m_GizmoColor;
  ezUInt32 m_uiUniqueID;
  bool m_bIsPickable;
};

class ezGizmoComponent;
class ezGizmoComponentManager : public ezComponentManager<ezGizmoComponent, ezBlockStorageType::FreeList>
{
public:
  ezGizmoComponentManager(ezWorld* pWorld);

  ezUInt32 m_uiHighlightID = 0;
};

/// \brief Used by the editor to render gizmo meshes.
///
/// Gizmos use special shaders to have constant screen-space size and swap geometry towards the viewer,
/// so their culling is non-trivial. This component takes care of that and of the highlight color.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGizmoComponent : public ezMeshComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezGizmoComponent, ezMeshComponent, ezGizmoComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezMeshComponentBase

protected:
  virtual ezMeshRenderData* CreateRenderData(const ezRenderDataManager* pRenderDataManager) const override;
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // ezGizmoComponent

public:
  ezGizmoComponent();
  ~ezGizmoComponent();

  ezColor m_GizmoColor = ezColor::White;
  bool m_bIsPickable = true;
  ezDynamicArray<ezVec3> m_Lines;
};
