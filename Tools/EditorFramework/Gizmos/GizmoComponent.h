#pragma once

#include <EditorFramework/Plugin.h>
#include <RendererCore/Meshes/MeshComponent.h>

class EZ_EDITORFRAMEWORK_DLL ezGizmoRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoRenderData, ezMeshRenderData);

public:
  ezColor m_GizmoColor;
  bool m_bUseDepthPrepass;
};

class ezGizmoComponent;
typedef ezComponentManager<ezGizmoComponent, ezBlockStorageType::FreeList> ezGizmoComponentManager;

class EZ_EDITORFRAMEWORK_DLL ezGizmoComponent : public ezMeshComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezGizmoComponent, ezMeshComponent, ezGizmoComponentManager);

public:
  ezGizmoComponent();

  ezColor m_GizmoColor;
  bool m_bUseDepthPrepass;

protected:
  virtual ezMeshRenderData* CreateRenderData(ezUInt32 uiBatchId) const override;

};

